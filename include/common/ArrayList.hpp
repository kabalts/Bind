#pragma once

#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <initializer_list>

template <typename T>
class ArrayList {
public:
    class Iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        Iterator() : m_ptr(nullptr) {}
        explicit Iterator(T* ptr) : m_ptr(ptr) {}

        reference operator*()  const { return *m_ptr; }
        pointer   operator->() const { return  m_ptr; }

        Iterator& operator++()    { ++m_ptr; return *this; }
        Iterator  operator++(int) { Iterator tmp = *this; ++m_ptr; return tmp; }
        Iterator& operator--()    { --m_ptr; return *this; }
        Iterator  operator--(int) { Iterator tmp = *this; --m_ptr; return tmp; }

        Iterator& operator+=(difference_type n) { m_ptr += n; return *this; }
        Iterator& operator-=(difference_type n) { m_ptr -= n; return *this; }

        Iterator operator+(difference_type n) const { return Iterator(m_ptr + n); }
        Iterator operator-(difference_type n) const { return Iterator(m_ptr - n); }

        difference_type operator-(const Iterator& other) const { return m_ptr - other.m_ptr; }

        reference operator[](difference_type n) const { return m_ptr[n]; }

        friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; }
        friend bool operator< (const Iterator& a, const Iterator& b) { return a.m_ptr <  b.m_ptr; }
        friend bool operator> (const Iterator& a, const Iterator& b) { return a.m_ptr >  b.m_ptr; }
        friend bool operator<=(const Iterator& a, const Iterator& b) { return a.m_ptr <= b.m_ptr; }
        friend bool operator>=(const Iterator& a, const Iterator& b) { return a.m_ptr >= b.m_ptr; }

        friend Iterator operator+(difference_type n, const Iterator& it) { return Iterator(it.m_ptr + n); }

    private:
        T* m_ptr;
    };

    class ConstIterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = const T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        ConstIterator() : m_ptr(nullptr) {}
        explicit ConstIterator(const T* ptr) : m_ptr(ptr) {}
        ConstIterator(Iterator it) : m_ptr(&(*it)) {}

        reference operator*()  const { return *m_ptr; }
        pointer   operator->() const { return  m_ptr; }

        ConstIterator& operator++()    { ++m_ptr; return *this; }
        ConstIterator  operator++(int) { ConstIterator tmp = *this; ++m_ptr; return tmp; }
        ConstIterator& operator--()    { --m_ptr; return *this; }
        ConstIterator  operator--(int) { ConstIterator tmp = *this; --m_ptr; return tmp; }

        ConstIterator& operator+=(difference_type n) { m_ptr += n; return *this; }
        ConstIterator& operator-=(difference_type n) { m_ptr -= n; return *this; }

        ConstIterator operator+(difference_type n) const { return ConstIterator(m_ptr + n); }
        ConstIterator operator-(difference_type n) const { return ConstIterator(m_ptr - n); }

        difference_type operator-(const ConstIterator& other) const { return m_ptr - other.m_ptr; }

        reference operator[](difference_type n) const { return m_ptr[n]; }

        friend bool operator==(const ConstIterator& a, const ConstIterator& b) { return a.m_ptr == b.m_ptr; }
        friend bool operator!=(const ConstIterator& a, const ConstIterator& b) { return a.m_ptr != b.m_ptr; }
        friend bool operator< (const ConstIterator& a, const ConstIterator& b) { return a.m_ptr <  b.m_ptr; }
        friend bool operator> (const ConstIterator& a, const ConstIterator& b) { return a.m_ptr >  b.m_ptr; }
        friend bool operator<=(const ConstIterator& a, const ConstIterator& b) { return a.m_ptr <= b.m_ptr; }
        friend bool operator>=(const ConstIterator& a, const ConstIterator& b) { return a.m_ptr >= b.m_ptr; }

        friend ConstIterator operator+(difference_type n, const ConstIterator& it) { return ConstIterator(it.m_ptr + n); }

    private:
        const T* m_ptr;
    };

    using iterator       = Iterator;
    using const_iterator = ConstIterator;

    ArrayList() : m_data(nullptr), m_size(0), m_capacity(0) {}

    explicit ArrayList(size_t initial_capacity) : m_data(nullptr), m_size(0), m_capacity(0) {
        if (initial_capacity > 0) {
            m_data = static_cast<T*>(::operator new(initial_capacity * sizeof(T)));
            m_capacity = initial_capacity;
        }
    }

    ArrayList(std::initializer_list<T> il) : m_data(nullptr), m_size(0), m_capacity(0) {
        reserve(il.size());
        for (const auto& val : il) {
            push_back(val);
        }
    }

    ArrayList(const ArrayList& other) : m_data(nullptr), m_size(0), m_capacity(0) {
        reserve(other.m_size);
        for (size_t i = 0; i < other.m_size; ++i) {
            std::construct_at(m_data + i, other.m_data[i]);
        }
        m_size = other.m_size;
    }

    ArrayList(ArrayList&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
        other.m_data     = nullptr;
        other.m_size     = 0;
        other.m_capacity = 0;
    }

    ~ArrayList() {
        clear();
        ::operator delete(m_data);
    }

    ArrayList& operator=(const ArrayList& other) {
        if (this != &other) {
            ArrayList tmp(other);
            swap(tmp);
        }
        return *this;
    }

    ArrayList& operator=(ArrayList&& other) noexcept {
        if (this != &other) {
            clear();
            ::operator delete(m_data);
            m_data     = other.m_data;
            m_size     = other.m_size;
            m_capacity = other.m_capacity;
            other.m_data     = nullptr;
            other.m_size     = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    void swap(ArrayList& other) noexcept {
        T*    tmp_data     = m_data;
        size_t tmp_size     = m_size;
        size_t tmp_capacity = m_capacity;
        m_data     = other.m_data;
        m_size     = other.m_size;
        m_capacity = other.m_capacity;
        other.m_data     = tmp_data;
        other.m_size     = tmp_size;
        other.m_capacity = tmp_capacity;
    }

    void push_back(const T& value) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
        }
        std::construct_at(m_data + m_size, value);
        ++m_size;
    }

    void push_back(T&& value) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
        }
        std::construct_at(m_data + m_size, std::move(value));
        ++m_size;
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
        }
        T* ptr = std::construct_at(m_data + m_size, std::forward<Args>(args)...);
        ++m_size;
        return *ptr;
    }

    void pop_back() {
        if (m_size > 0) {
            --m_size;
            std::destroy_at(m_data + m_size);
        }
    }

    T& operator[](size_t index)             { return m_data[index]; }
    const T& operator[](size_t index) const { return m_data[index]; }

    T&       at(size_t index) {
        if (index >= m_size) throw std::out_of_range("ArrayList::at: index out of range");
        return m_data[index];
    }
    const T& at(size_t index) const {
        if (index >= m_size) throw std::out_of_range("ArrayList::at: index out of range");
        return m_data[index];
    }

    T&       front()       { return m_data[0]; }
    const T& front() const { return m_data[0]; }
    T&       back()        { return m_data[m_size - 1]; }
    const T& back()  const { return m_data[m_size - 1]; }

    T*       data()       { return m_data; }
    const T* data() const { return m_data; }

    size_t size()     const { return m_size; }
    size_t capacity() const { return m_capacity; }
    bool   empty()    const { return m_size == 0; }

    void reserve(size_t new_capacity) {
        if (new_capacity <= m_capacity) return;
        T* new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (m_data && m_size > 0) {
                std::memcpy(new_data, m_data, m_size * sizeof(T));
            }
        } else {
            for (size_t i = 0; i < m_size; ++i) {
                std::construct_at(new_data + i, std::move(m_data[i]));
                std::destroy_at(m_data + i);
            }
        }
        ::operator delete(m_data);
        m_data = new_data;
        m_capacity = new_capacity;
    }

    void shrink_to_fit() {
        if (m_size == m_capacity) return;
        if (m_size == 0) {
            ::operator delete(m_data);
            m_data = nullptr;
            m_capacity = 0;
            return;
        }
        T* new_data = static_cast<T*>(::operator new(m_size * sizeof(T)));
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(new_data, m_data, m_size * sizeof(T));
        } else {
            for (size_t i = 0; i < m_size; ++i) {
                std::construct_at(new_data + i, std::move(m_data[i]));
                std::destroy_at(m_data + i);
            }
        }
        ::operator delete(m_data);
        m_data = new_data;
        m_capacity = m_size;
    }

    void clear() {
        for (size_t i = 0; i < m_size; ++i) {
            std::destroy_at(m_data + i);
        }
        m_size = 0;
    }

    void resize(size_t new_size) {
        if (new_size < m_size) {
            for (size_t i = new_size; i < m_size; ++i) {
                std::destroy_at(m_data + i);
            }
        } else if (new_size > m_size) {
            reserve(new_size);
            for (size_t i = m_size; i < new_size; ++i) {
                std::construct_at(m_data + i);
            }
        }
        m_size = new_size;
    }

    void resize(size_t new_size, const T& value) {
        if (new_size < m_size) {
            for (size_t i = new_size; i < m_size; ++i) {
                std::destroy_at(m_data + i);
            }
        } else if (new_size > m_size) {
            reserve(new_size);
            for (size_t i = m_size; i < new_size; ++i) {
                std::construct_at(m_data + i, value);
            }
        }
        m_size = new_size;
    }

    iterator begin() { return Iterator(m_data); }
    iterator end()   { return Iterator(m_data + m_size); }

    const_iterator begin() const { return ConstIterator(m_data); }
    const_iterator end()   const { return ConstIterator(m_data + m_size); }

    const_iterator cbegin() const { return ConstIterator(m_data); }
    const_iterator cend()   const { return ConstIterator(m_data + m_size); }

    iterator insert(const_iterator pos, const T& value) {
        size_t idx = pos - cbegin();
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
        }
        for (size_t i = m_size; i > idx; --i) {
            std::construct_at(m_data + i, std::move(m_data[i - 1]));
            std::destroy_at(m_data + i - 1);
        }
        std::construct_at(m_data + idx, value);
        ++m_size;
        return Iterator(m_data + idx);
    }

    iterator insert(const_iterator pos, T&& value) {
        size_t idx = pos - cbegin();
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
        }
        for (size_t i = m_size; i > idx; --i) {
            std::construct_at(m_data + i, std::move(m_data[i - 1]));
            std::destroy_at(m_data + i - 1);
        }
        std::construct_at(m_data + idx, std::move(value));
        ++m_size;
        return Iterator(m_data + idx);
    }

    iterator erase(const_iterator pos) {
        size_t idx = pos - cbegin();
        std::destroy_at(m_data + idx);
        for (size_t i = idx; i < m_size - 1; ++i) {
            std::construct_at(m_data + i, std::move(m_data[i + 1]));
            std::destroy_at(m_data + i + 1);
        }
        --m_size;
        return Iterator(m_data + idx);
    }

    iterator erase(const_iterator first, const_iterator last) {
        size_t idx_first = first - cbegin();
        size_t idx_last  = last  - cbegin();
        size_t count = idx_last - idx_first;
        for (size_t i = idx_first; i < idx_last; ++i) {
            std::destroy_at(m_data + i);
        }
        for (size_t i = idx_first; i < m_size - count; ++i) {
            std::construct_at(m_data + i, std::move(m_data[i + count]));
            std::destroy_at(m_data + i + count);
        }
        m_size -= count;
        return Iterator(m_data + idx_first);
    }

private:
    T*     m_data;
    size_t m_size;
    size_t m_capacity;
};

template <typename T>
void swap(ArrayList<T>& a, ArrayList<T>& b) noexcept {
    a.swap(b);
}
