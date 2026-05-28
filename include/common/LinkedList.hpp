#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <utility>
#include <stdexcept>
#include <initializer_list>

template <typename T>
class LinkedList {
private:
    struct Node {
        T     data;
        Node* prev;
        Node* next;

        template <typename... Args>
        Node(Node* p, Node* n, Args&&... args)
            : data(std::forward<Args>(args)...), prev(p), next(n) {}

        Node(Node* p, Node* n, const T& value)
            : data(value), prev(p), next(n) {}

        Node(Node* p, Node* n, T&& value)
            : data(std::move(value)), prev(p), next(n) {}
    };

public:
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        Iterator() : m_node(nullptr) {}
        explicit Iterator(Node* node) : m_node(node) {}

        reference operator*()  const { return m_node->data; }
        pointer   operator->() const { return &m_node->data; }

        Iterator& operator++()    { m_node = m_node->next; return *this; }
        Iterator  operator++(int) { Iterator tmp = *this; m_node = m_node->next; return tmp; }
        Iterator& operator--()    { m_node = m_node->prev; return *this; }
        Iterator  operator--(int) { Iterator tmp = *this; m_node = m_node->prev; return tmp; }

        friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_node == b.m_node; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_node != b.m_node; }

        Node* node() const { return m_node; }

    private:
        Node* m_node;
    };

    class ConstIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = const T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        ConstIterator() : m_node(nullptr) {}
        explicit ConstIterator(const Node* node) : m_node(node) {}
        ConstIterator(Iterator it) : m_node(it.node()) {}

        reference operator*()  const { return m_node->data; }
        pointer   operator->() const { return &m_node->data; }

        ConstIterator& operator++()    { m_node = m_node->next; return *this; }
        ConstIterator  operator++(int) { ConstIterator tmp = *this; m_node = m_node->next; return tmp; }
        ConstIterator& operator--()    { m_node = m_node->prev; return *this; }
        ConstIterator  operator--(int) { ConstIterator tmp = *this; m_node = m_node->prev; return tmp; }

        friend bool operator==(const ConstIterator& a, const ConstIterator& b) { return a.m_node == b.m_node; }
        friend bool operator!=(const ConstIterator& a, const ConstIterator& b) { return a.m_node != b.m_node; }

        const Node* node() const { return m_node; }

    private:
        const Node* m_node;
    };

    using iterator       = Iterator;
    using const_iterator = ConstIterator;

    LinkedList() {
        m_sentinel = static_cast<Node*>(::operator new(sizeof(Node)));
        m_sentinel->prev = m_sentinel;
        m_sentinel->next = m_sentinel;
        m_size = 0;
    }

    LinkedList(std::initializer_list<T> il) : LinkedList() {
        for (const auto& val : il) {
            push_back(val);
        }
    }

    LinkedList(const LinkedList& other) : LinkedList() {
        for (const auto& val : other) {
            push_back(val);
        }
    }

    LinkedList(LinkedList&& other) noexcept
        : m_sentinel(other.m_sentinel), m_size(other.m_size) {
        other.m_sentinel = nullptr;
        other.m_size = 0;
    }

    ~LinkedList() {
        if (m_sentinel) {
            clear();
            ::operator delete(m_sentinel);
        }
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            LinkedList tmp(other);
            swap(tmp);
        }
        return *this;
    }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            if (m_sentinel) {
                ::operator delete(m_sentinel);
            }
            m_sentinel = other.m_sentinel;
            m_size     = other.m_size;
            other.m_sentinel = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    void swap(LinkedList& other) noexcept {
        Node*   tmp_sentinel = m_sentinel;
        size_t  tmp_size     = m_size;
        m_sentinel = other.m_sentinel;
        m_size     = other.m_size;
        other.m_sentinel = tmp_sentinel;
        other.m_size     = tmp_size;
    }

    void push_back(const T& value) {
        Node* node = static_cast<Node*>(::operator new(sizeof(Node)));
        std::construct_at(node, m_sentinel->prev, m_sentinel, value);
        m_sentinel->prev->next = node;
        m_sentinel->prev = node;
        ++m_size;
    }

    void push_back(T&& value) {
        Node* node = static_cast<Node*>(::operator new(sizeof(Node)));
        std::construct_at(node, m_sentinel->prev, m_sentinel, std::move(value));
        m_sentinel->prev->next = node;
        m_sentinel->prev = node;
        ++m_size;
    }

    void push_front(const T& value) {
        Node* node = static_cast<Node*>(::operator new(sizeof(Node)));
        std::construct_at(node, m_sentinel, m_sentinel->next, value);
        m_sentinel->next->prev = node;
        m_sentinel->next = node;
        ++m_size;
    }

    void push_front(T&& value) {
        Node* node = static_cast<Node*>(::operator new(sizeof(Node)));
        std::construct_at(node, m_sentinel, m_sentinel->next, std::move(value));
        m_sentinel->next->prev = node;
        m_sentinel->next = node;
        ++m_size;
    }

    void pop_back() {
        if (m_size == 0) return;
        Node* node = m_sentinel->prev;
        node->prev->next = m_sentinel;
        m_sentinel->prev = node->prev;
        std::destroy_at(node);
        ::operator delete(node);
        --m_size;
    }

    void pop_front() {
        if (m_size == 0) return;
        Node* node = m_sentinel->next;
        m_sentinel->next = node->next;
        node->next->prev = m_sentinel;
        std::destroy_at(node);
        ::operator delete(node);
        --m_size;
    }

    iterator insert(const_iterator pos, const T& value) {
        Node* node = pos.node() ? const_cast<Node*>(pos.node()) : m_sentinel;
        Node* new_node = static_cast<Node*>(::operator new(sizeof(Node)));
        std::construct_at(new_node, node->prev, node, value);
        node->prev->next = new_node;
        node->prev = new_node;
        ++m_size;
        return Iterator(new_node);
    }

    iterator insert(const_iterator pos, T&& value) {
        Node* node = pos.node() ? const_cast<Node*>(pos.node()) : m_sentinel;
        Node* new_node = static_cast<Node*>(::operator new(sizeof(Node)));
        std::construct_at(new_node, node->prev, node, std::move(value));
        node->prev->next = new_node;
        node->prev = new_node;
        ++m_size;
        return Iterator(new_node);
    }

    iterator erase(const_iterator pos) {
        Node* node = const_cast<Node*>(pos.node());
        if (!node || node == m_sentinel) return end();
        Node* next_node = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        std::destroy_at(node);
        ::operator delete(node);
        --m_size;
        return Iterator(next_node);
    }

    iterator erase(const_iterator first, const_iterator last) {
        Node* f = const_cast<Node*>(first.node());
        Node* l = const_cast<Node*>(last.node());
        if (!f || f == m_sentinel) return end();

        Node* prev_node = f->prev;
        Node* curr = f;
        while (curr != l) {
            Node* next = curr->next;
            std::destroy_at(curr);
            ::operator delete(curr);
            --m_size;
            curr = next;
        }
        prev_node->next = l;
        l->prev = prev_node;
        return Iterator(l);
    }

    T& front() { return m_sentinel->next->data; }
    const T& front() const { return m_sentinel->next->data; }

    T& back() { return m_sentinel->prev->data; }
    const T& back() const { return m_sentinel->prev->data; }

    size_t size()  const { return m_size; }
    bool   empty() const { return m_size == 0; }

    void clear() {
        Node* curr = m_sentinel->next;
        while (curr != m_sentinel) {
            Node* next = curr->next;
            std::destroy_at(curr);
            ::operator delete(curr);
            curr = next;
        }
        m_sentinel->prev = m_sentinel;
        m_sentinel->next = m_sentinel;
        m_size = 0;
    }

    iterator begin() { return Iterator(m_sentinel->next); }
    iterator end()   { return Iterator(m_sentinel); }

    const_iterator begin() const { return ConstIterator(m_sentinel->next); }
    const_iterator end()   const { return ConstIterator(m_sentinel); }

    const_iterator cbegin() const { return ConstIterator(m_sentinel->next); }
    const_iterator cend()   const { return ConstIterator(m_sentinel); }

    void remove(const T& value) {
        Node* curr = m_sentinel->next;
        while (curr != m_sentinel) {
            Node* next = curr->next;
            if (curr->data == value) {
                curr->prev->next = curr->next;
                curr->next->prev = curr->prev;
                std::destroy_at(curr);
                ::operator delete(curr);
                --m_size;
            }
            curr = next;
        }
    }

private:
    Node*  m_sentinel;
    size_t m_size;
};

template <typename T>
void swap(LinkedList<T>& a, LinkedList<T>& b) noexcept {
    a.swap(b);
}
