#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <new>
#include "common/String.hpp"

template <typename K, typename V>
class BPlusTree {
    static_assert(std::is_trivially_copyable_v<K>, "K must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<V>, "V must be trivially copyable");

    static constexpr int PAGE_SIZE = 4096;

    static constexpr size_t HEADER_IS_LEAF     = 0;
    static constexpr size_t HEADER_NUM_KEYS    = 4;
    static constexpr size_t HEADER_NEXT_LEAF   = 8;
    static constexpr size_t HEADER_PARENT_PAGE = 16;
    static constexpr size_t LEAF_DATA_OFFSET   = 24;
    static constexpr size_t INTERNAL_FIRST_CHILD_OFFSET = 24;
    static constexpr size_t INTERNAL_DATA_OFFSET = 32;
    static constexpr size_t KEY_SIZE   = sizeof(K);
    static constexpr size_t VAL_SIZE   = sizeof(V);
    static constexpr size_t CHILD_SIZE = sizeof(int64_t);
    static constexpr size_t LEAF_ENTRY_SIZE    = KEY_SIZE + VAL_SIZE;
    static constexpr size_t INTERNAL_ENTRY_SIZE = KEY_SIZE + CHILD_SIZE;

    static constexpr int MAX_LEAF_KEYS     = static_cast<int>((PAGE_SIZE - LEAF_DATA_OFFSET) / LEAF_ENTRY_SIZE);
    static constexpr int MAX_INTERNAL_KEYS = static_cast<int>((PAGE_SIZE - INTERNAL_DATA_OFFSET) / INTERNAL_ENTRY_SIZE);

    static constexpr int MIN_LEAF_KEYS     = (MAX_LEAF_KEYS + 1) / 2;
    static constexpr int MIN_INTERNAL_KEYS = (MAX_INTERNAL_KEYS + 1) / 2;

    static constexpr int64_t NULL_PAGE = -1;
    static constexpr int64_t METADATA_PAGE_NO = 0;
    static constexpr int64_t FIRST_DATA_PAGE  = 1;

public:
    BPlusTree();
    ~BPlusTree();

    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;
    BPlusTree(BPlusTree&& other) noexcept;
    BPlusTree& operator=(BPlusTree&& other) noexcept;

    bool open(const char* filename);
    void close();
    bool is_open() const;

    bool insert(const K& key, const V& value);
    bool remove(const K& key);
    bool search(const K& key, V& value) const;
    int  range_search(const K& low, const K& high, V* results, int max_results) const;

    const String& filename() const { return m_filename; }

private:
    struct Page {
        unsigned char data[PAGE_SIZE];

        int32_t  is_leaf()          const { return read_int32(HEADER_IS_LEAF); }
        void     set_is_leaf(int32_t v)    { write_int32(HEADER_IS_LEAF, v); }
        int32_t  num_keys()         const { return read_int32(HEADER_NUM_KEYS); }
        void     set_num_keys(int32_t v)   { write_int32(HEADER_NUM_KEYS, v); }
        int64_t  next_leaf()        const { return read_int64(HEADER_NEXT_LEAF); }
        void     set_next_leaf(int64_t v)  { write_int64(HEADER_NEXT_LEAF, v); }
        int64_t  parent_page()      const { return read_int64(HEADER_PARENT_PAGE); }
        void     set_parent_page(int64_t v){ write_int64(HEADER_PARENT_PAGE, v); }

        int64_t  first_child() const {
            return read_int64(INTERNAL_FIRST_CHILD_OFFSET);
        }
        void set_first_child(int64_t v) {
            write_int64(INTERNAL_FIRST_CHILD_OFFSET, v);
        }

        K key_at(int idx) const {
            return read_key(LEAF_DATA_OFFSET + idx * LEAF_ENTRY_SIZE);
        }
        void set_key_at(int idx, const K& k) {
            write_key(LEAF_DATA_OFFSET + idx * LEAF_ENTRY_SIZE, k);
        }
        V value_at(int idx) const {
            return read_value(LEAF_DATA_OFFSET + idx * LEAF_ENTRY_SIZE + KEY_SIZE);
        }
        void set_value_at(int idx, const V& v) {
            write_value(LEAF_DATA_OFFSET + idx * LEAF_ENTRY_SIZE + KEY_SIZE, v);
        }

        K internal_key_at(int idx) const {
            return read_key(INTERNAL_DATA_OFFSET + idx * INTERNAL_ENTRY_SIZE);
        }
        void set_internal_key_at(int idx, const K& k) {
            write_key(INTERNAL_DATA_OFFSET + idx * INTERNAL_ENTRY_SIZE, k);
        }
        int64_t child_at(int idx) const {
            return read_int64(INTERNAL_DATA_OFFSET + idx * INTERNAL_ENTRY_SIZE + KEY_SIZE);
        }
        void set_child_at(int idx, int64_t v) {
            write_int64(INTERNAL_DATA_OFFSET + idx * INTERNAL_ENTRY_SIZE + KEY_SIZE, v);
        }

        int64_t child_for_key(int num_keys, const K& key) const {
            if (key < internal_key_at(0)) return first_child();
            int left = 0, right = num_keys - 1;
            while (left < right) {
                int mid = (left + right + 1) / 2;
                if (internal_key_at(mid) <= key)
                    left = mid;
                else
                    right = mid - 1;
            }
            return child_at(left);
        }

        int find_key_pos(int num_keys_now, const K& key) const {
            int left = 0, right = num_keys_now - 1;
            while (left <= right) {
                int mid = (left + right) / 2;
                K mk = key_at(mid);
                if (mk == key) return mid;
                if (mk < key) left = mid + 1;
                else right = mid - 1;
            }
            return left;
        }

        int find_internal_key_pos(int num_keys_now, const K& key) const {
            int left = 0, right = num_keys_now - 1;
            while (left <= right) {
                int mid = (left + right) / 2;
                if (internal_key_at(mid) == key) return mid;
                if (internal_key_at(mid) < key) left = mid + 1;
                else right = mid - 1;
            }
            return left;
        }

        void insert_leaf_entry(int pos, const K& key, const V& value, int& num_keys_now) {
            for (int i = num_keys_now; i > pos; --i) {
                set_key_at(i, key_at(i - 1));
                set_value_at(i, value_at(i - 1));
            }
            set_key_at(pos, key);
            set_value_at(pos, value);
            ++num_keys_now;
        }

        void remove_leaf_entry(int pos, int& num_keys_now) {
            for (int i = pos; i < num_keys_now - 1; ++i) {
                set_key_at(i, key_at(i + 1));
                set_value_at(i, value_at(i + 1));
            }
            --num_keys_now;
        }

        void insert_internal_entry(int pos, const K& key, int64_t child, int& num_keys_now) {
            for (int i = num_keys_now; i > pos; --i) {
                set_internal_key_at(i, internal_key_at(i - 1));
                set_child_at(i, child_at(i - 1));
            }
            set_internal_key_at(pos, key);
            set_child_at(pos, child);
            ++num_keys_now;
        }

        void remove_internal_entry(int pos, int& num_keys_now) {
            for (int i = pos; i < num_keys_now - 1; ++i) {
                set_internal_key_at(i, internal_key_at(i + 1));
                set_child_at(i, child_at(i + 1));
            }
            --num_keys_now;
        }

        void zero_out() {
            std::memset(data, 0, PAGE_SIZE);
        }

    private:
        int32_t read_int32(size_t offset) const {
            int32_t val;
            std::memcpy(&val, data + offset, sizeof(int32_t));
            return val;
        }
        void write_int32(size_t offset, int32_t v) {
            std::memcpy(data + offset, &v, sizeof(int32_t));
        }
        int64_t read_int64(size_t offset) const {
            int64_t val;
            std::memcpy(&val, data + offset, sizeof(int64_t));
            return val;
        }
        void write_int64(size_t offset, int64_t v) {
            std::memcpy(data + offset, &v, sizeof(int64_t));
        }
        K read_key(size_t offset) const {
            K val;
            std::memcpy(&val, data + offset, KEY_SIZE);
            return val;
        }
        void write_key(size_t offset, const K& val) {
            std::memcpy(data + offset, &val, KEY_SIZE);
        }
        V read_value(size_t offset) const {
            V val;
            std::memcpy(&val, data + offset, VAL_SIZE);
            return val;
        }
        void write_value(size_t offset, const V& val) {
            std::memcpy(data + offset, &val, VAL_SIZE);
        }
    };

    FILE*   m_file;
    int64_t m_root_page;
    int64_t m_next_page;
    String  m_filename;
    bool    m_open;

    bool read_page(int64_t page_no, Page& page) const;
    bool write_page(int64_t page_no, const Page& page);
    int64_t allocate_page();
    bool read_metadata();
    bool write_metadata();

    int64_t find_leaf(const K& key) const;

    bool insert_into_leaf(int64_t leaf_page, Page& leaf, const K& key, const V& value);
    bool insert_into_parent(int64_t left_page, const K& key, int64_t right_page);

    bool remove_entry(int64_t page_no, Page& page, const K& key);
    bool coalesce_nodes(int64_t page_no, Page& page, int64_t neighbor_no, Page& neighbor,
                        int parent_idx, int64_t parent_no, const K& middle_key);
    bool redistribute_nodes(int64_t page_no, Page& page, int64_t neighbor_no, Page& neighbor,
                            int parent_idx, int64_t parent_no, bool from_left);

    void update_parent_pointers(int64_t page_no, int64_t new_parent);
    long file_offset(int64_t page_no) const;
};

template <typename K, typename V>
BPlusTree<K, V>::BPlusTree()
    : m_file(nullptr), m_root_page(NULL_PAGE), m_next_page(FIRST_DATA_PAGE),
      m_filename(), m_open(false) {}

template <typename K, typename V>
BPlusTree<K, V>::~BPlusTree() {
    close();
}

template <typename K, typename V>
BPlusTree<K, V>::BPlusTree(BPlusTree&& other) noexcept
    : m_file(other.m_file), m_root_page(other.m_root_page),
      m_next_page(other.m_next_page), m_filename(std::move(other.m_filename)),
      m_open(other.m_open) {
    other.m_file = nullptr;
    other.m_open = false;
    other.m_root_page = NULL_PAGE;
    other.m_next_page = FIRST_DATA_PAGE;
}

template <typename K, typename V>
BPlusTree<K, V>& BPlusTree<K, V>::operator=(BPlusTree&& other) noexcept {
    if (this != &other) {
        close();
        m_file = other.m_file;
        m_root_page = other.m_root_page;
        m_next_page = other.m_next_page;
        m_filename = std::move(other.m_filename);
        m_open = other.m_open;
        other.m_file = nullptr;
        other.m_open = false;
        other.m_root_page = NULL_PAGE;
        other.m_next_page = FIRST_DATA_PAGE;
    }
    return *this;
}

template <typename K, typename V>
bool BPlusTree<K, V>::open(const char* filename) {
    close();
    m_filename = filename;

    FILE* test = std::fopen(filename, "rb");
    if (test) {
        std::fclose(test);
        m_file = std::fopen(filename, "r+b");
        if (!m_file) return false;
        if (!read_metadata()) {
            std::fclose(m_file);
            m_file = nullptr;
            return false;
        }
    } else {
        m_file = std::fopen(filename, "w+b");
        if (!m_file) return false;

        m_root_page = NULL_PAGE;
        m_next_page = FIRST_DATA_PAGE;

        Page meta;
        meta.zero_out();
        meta.set_is_leaf(0);
        meta.set_num_keys(0);
        meta.set_next_leaf(NULL_PAGE);
        meta.set_parent_page(NULL_PAGE);
        meta.set_first_child(m_root_page);
        if (!write_page(METADATA_PAGE_NO, meta)) {
            std::fclose(m_file);
            m_file = nullptr;
            return false;
        }
        std::fflush(m_file);
    }

    m_open = true;
    return true;
}

template <typename K, typename V>
void BPlusTree<K, V>::close() {
    if (m_file) {
        write_metadata();
        std::fflush(m_file);
        std::fclose(m_file);
        m_file = nullptr;
    }
    m_open = false;
}

template <typename K, typename V>
bool BPlusTree<K, V>::is_open() const {
    return m_open;
}

template <typename K, typename V>
long BPlusTree<K, V>::file_offset(int64_t page_no) const {
    return static_cast<long>(page_no * PAGE_SIZE);
}

template <typename K, typename V>
bool BPlusTree<K, V>::read_page(int64_t page_no, Page& page) const {
    if (!m_file || page_no < 0) return false;
    if (std::fseek(m_file, file_offset(page_no), SEEK_SET) != 0) return false;
    size_t n = std::fread(page.data, 1, PAGE_SIZE, m_file);
    return n == PAGE_SIZE;
}

template <typename K, typename V>
bool BPlusTree<K, V>::write_page(int64_t page_no, const Page& page) {
    if (!m_file || page_no < 0) return false;
    if (std::fseek(m_file, file_offset(page_no), SEEK_SET) != 0) return false;
    size_t n = std::fwrite(page.data, 1, PAGE_SIZE, m_file);
    if (n != PAGE_SIZE) return false;
    std::fflush(m_file);
    return true;
}

template <typename K, typename V>
int64_t BPlusTree<K, V>::allocate_page() {
    int64_t page = m_next_page;
    ++m_next_page;

    Page new_page;
    new_page.zero_out();
    write_page(page, new_page);

    return page;
}

template <typename K, typename V>
bool BPlusTree<K, V>::read_metadata() {
    Page meta;
    if (!read_page(METADATA_PAGE_NO, meta)) return false;
    m_root_page = meta.first_child();
    m_next_page = meta.next_leaf();
    if (m_next_page < FIRST_DATA_PAGE) {
        m_next_page = FIRST_DATA_PAGE;
    }
    return true;
}

template <typename K, typename V>
bool BPlusTree<K, V>::write_metadata() {
    if (!m_file) return false;
    Page meta;
    meta.zero_out();
    meta.set_is_leaf(0);
    meta.set_num_keys(0);
    meta.set_first_child(m_root_page);
    meta.set_next_leaf(m_next_page);
    meta.set_parent_page(NULL_PAGE);
    return write_page(METADATA_PAGE_NO, meta);
}

template <typename K, typename V>
int64_t BPlusTree<K, V>::find_leaf(const K& key) const {
    if (m_root_page == NULL_PAGE) return NULL_PAGE;

    int64_t page_no = m_root_page;
    Page page;
    if (!read_page(page_no, page)) return NULL_PAGE;

    while (!page.is_leaf()) {
        int nk = page.num_keys();
        int64_t child;
        if (nk == 0) {
            child = page.first_child();
        } else {
            child = page.child_for_key(nk, key);
        }
        page_no = child;
        if (!read_page(page_no, page)) return NULL_PAGE;
    }
    return page_no;
}

template <typename K, typename V>
bool BPlusTree<K, V>::search(const K& key, V& value) const {
    int64_t leaf_no = find_leaf(key);
    if (leaf_no == NULL_PAGE) return false;

    Page leaf;
    if (!read_page(leaf_no, leaf)) return false;

    int nk = leaf.num_keys();
    int pos = leaf.find_key_pos(nk, key);
    if (pos < nk && leaf.key_at(pos) == key) {
        value = leaf.value_at(pos);
        return true;
    }
    return false;
}

template <typename K, typename V>
int BPlusTree<K, V>::range_search(const K& low, const K& high, V* results, int max_results) const {
    if (m_root_page == NULL_PAGE) return 0;
    int count = 0;

    int64_t leaf_no = find_leaf(low);
    if (leaf_no == NULL_PAGE) return 0;

    while (leaf_no != NULL_PAGE && count < max_results) {
        Page leaf;
        if (!read_page(leaf_no, leaf)) break;

        int nk = leaf.num_keys();
        for (int i = 0; i < nk && count < max_results; ++i) {
            K k = leaf.key_at(i);
            if (k < low) continue;
            if (k > high) return count;
            results[count] = leaf.value_at(i);
            ++count;
        }

        if (leaf.key_at(nk - 1) > high) break;
        leaf_no = leaf.next_leaf();
    }
    return count;
}

template <typename K, typename V>
bool BPlusTree<K, V>::insert(const K& key, const V& value) {
    if (!m_open) return false;

    if (m_root_page == NULL_PAGE) {
        m_root_page = allocate_page();
        Page root;
        root.zero_out();
        root.set_is_leaf(1);
        root.set_num_keys(0);
        root.set_next_leaf(NULL_PAGE);
        root.set_parent_page(NULL_PAGE);
        write_page(m_root_page, root);
    }

    int64_t leaf_no = find_leaf(key);
    Page leaf;
    if (!read_page(leaf_no, leaf)) return false;

    if (insert_into_leaf(leaf_no, leaf, key, value)) {
        write_metadata();
        return true;
    }

    return false;
}

template <typename K, typename V>
bool BPlusTree<K, V>::insert_into_leaf(int64_t leaf_page_no, Page& leaf, const K& key, const V& value) {
    int nk = leaf.num_keys();
    int pos = leaf.find_key_pos(nk, key);

    if (pos < nk && leaf.key_at(pos) == key) {
        leaf.set_value_at(pos, value);
        write_page(leaf_page_no, leaf);
        return true;
    }

    if (nk < MAX_LEAF_KEYS) {
        leaf.insert_leaf_entry(pos, key, value, nk);
        leaf.set_num_keys(nk);
        write_page(leaf_page_no, leaf);
        return true;
    }

    int64_t new_leaf_no = allocate_page();
    Page new_leaf;
    new_leaf.zero_out();
    new_leaf.set_is_leaf(1);
    new_leaf.set_num_keys(0);
    new_leaf.set_next_leaf(leaf.next_leaf());
    new_leaf.set_parent_page(leaf.parent_page());

    int threshold = (MAX_LEAF_KEYS + 1) / 2;
    int split_point;
    K split_key;

    if (pos < threshold) {
        for (int i = threshold - 1; i < nk; ++i) {
            new_leaf.set_key_at(i - (threshold - 1), leaf.key_at(i));
            new_leaf.set_value_at(i - (threshold - 1), leaf.value_at(i));
        }
        new_leaf.set_num_keys(nk - (threshold - 1));
        for (int i = nk; i > pos; --i) {
            leaf.set_key_at(i, leaf.key_at(i - 1));
            leaf.set_value_at(i, leaf.value_at(i - 1));
        }
        leaf.set_key_at(pos, key);
        leaf.set_value_at(pos, value);
        leaf.set_num_keys(threshold);
        split_key = new_leaf.key_at(0);
    } else {
        int new_pos = pos - threshold;
        for (int i = threshold; i < pos; ++i) {
            new_leaf.set_key_at(i - threshold, leaf.key_at(i));
            new_leaf.set_value_at(i - threshold, leaf.value_at(i));
        }
        new_leaf.set_key_at(new_pos, key);
        new_leaf.set_value_at(new_pos, value);
        for (int i = pos; i < nk; ++i) {
            new_leaf.set_key_at(i - threshold + 1, leaf.key_at(i));
            new_leaf.set_value_at(i - threshold + 1, leaf.value_at(i));
        }
        new_leaf.set_num_keys(nk - threshold + 1);
        leaf.set_num_keys(threshold);
        split_key = new_leaf.key_at(0);
    }

    leaf.set_next_leaf(new_leaf_no);
    write_page(leaf_page_no, leaf);
    write_page(new_leaf_no, new_leaf);

    insert_into_parent(leaf_page_no, split_key, new_leaf_no);
    return true;
}

template <typename K, typename V>
bool BPlusTree<K, V>::insert_into_parent(int64_t left_page, const K& key, int64_t right_page) {
    Page left_node;
    if (!read_page(left_page, left_node)) return false;

    int64_t parent_no = left_node.parent_page();

    if (parent_no == NULL_PAGE) {
        int64_t new_root_no = allocate_page();
        Page new_root;
        new_root.zero_out();
        new_root.set_is_leaf(0);
        new_root.set_num_keys(1);
        new_root.set_first_child(left_page);
        new_root.set_internal_key_at(0, key);
        new_root.set_child_at(0, right_page);
        new_root.set_parent_page(NULL_PAGE);
        new_root.set_next_leaf(NULL_PAGE);
        write_page(new_root_no, new_root);

        left_node.set_parent_page(new_root_no);
        write_page(left_page, left_node);

        Page right_node;
        if (read_page(right_page, right_node)) {
            right_node.set_parent_page(new_root_no);
            write_page(right_page, right_node);
        }

        m_root_page = new_root_no;
        return true;
    }

    Page parent;
    if (!read_page(parent_no, parent)) return false;

    int pnk = parent.num_keys();
    int pos = parent.find_internal_key_pos(pnk, key);

    if (pnk < MAX_INTERNAL_KEYS) {
        parent.insert_internal_entry(pos, key, right_page, pnk);
        parent.set_num_keys(pnk);
        write_page(parent_no, parent);
        return true;
    }

    int64_t new_parent_no = allocate_page();
    Page new_parent;
    new_parent.zero_out();
    new_parent.set_is_leaf(0);
    new_parent.set_num_keys(0);
    new_parent.set_parent_page(parent.parent_page());
    new_parent.set_next_leaf(NULL_PAGE);

    int threshold = (MAX_INTERNAL_KEYS + 1) / 2;
    int mid_key_idx;
    K promoted_key;

    if (pos < threshold) {
        new_parent.set_first_child(parent.child_at(threshold - 1));
        for (int i = threshold; i < pnk; ++i) {
            int di = i - threshold;
            new_parent.set_internal_key_at(di, parent.internal_key_at(i));
            new_parent.set_child_at(di, parent.child_at(i));
        }
        new_parent.set_num_keys(pnk - threshold);
        mid_key_idx = threshold - 1;
        promoted_key = parent.internal_key_at(mid_key_idx);

        for (int i = pnk; i > pos; --i) {
            parent.set_internal_key_at(i, parent.internal_key_at(i - 1));
            parent.set_child_at(i, parent.child_at(i - 1));
        }
        parent.set_internal_key_at(pos, key);
        parent.set_child_at(pos, right_page);
        parent.set_num_keys(threshold);
    } else if (pos == threshold) {
        new_parent.set_first_child(right_page);
        for (int i = threshold; i < pnk; ++i) {
            int di = i - threshold;
            new_parent.set_internal_key_at(di, parent.internal_key_at(i));
            new_parent.set_child_at(di, parent.child_at(i));
        }
        new_parent.set_num_keys(pnk - threshold);
        parent.set_num_keys(threshold);
        promoted_key = key;
    } else {
        new_parent.set_first_child(parent.child_at(threshold));
        for (int i = threshold; i < pos; ++i) {
            int di = i - threshold;
            new_parent.set_internal_key_at(di, parent.internal_key_at(i));
            new_parent.set_child_at(di, parent.child_at(i));
        }
        new_parent.set_internal_key_at(pos - threshold, key);
        new_parent.set_child_at(pos - threshold, right_page);
        for (int i = pos; i < pnk; ++i) {
            int di = i - threshold + 1;
            new_parent.set_internal_key_at(di, parent.internal_key_at(i));
            new_parent.set_child_at(di, parent.child_at(i));
        }
        new_parent.set_num_keys(pnk - threshold + 1);
        parent.set_num_keys(threshold);
        promoted_key = parent.internal_key_at(threshold);
    }

    write_page(parent_no, parent);
    write_page(new_parent_no, new_parent);

    update_parent_pointers(right_page, parent_no);
    if (new_parent.first_child() != NULL_PAGE) {
        update_parent_pointers(new_parent.first_child(), new_parent_no);
    }
    for (int i = 0; i < new_parent.num_keys(); ++i) {
        update_parent_pointers(new_parent.child_at(i), new_parent_no);
    }

    insert_into_parent(parent_no, promoted_key, new_parent_no);
    return true;
}

template <typename K, typename V>
void BPlusTree<K, V>::update_parent_pointers(int64_t page_no, int64_t new_parent) {
    if (page_no == NULL_PAGE) return;
    Page page;
    if (!read_page(page_no, page)) return;
    page.set_parent_page(new_parent);
    write_page(page_no, page);
}

template <typename K, typename V>
bool BPlusTree<K, V>::remove(const K& key) {
    if (!m_open || m_root_page == NULL_PAGE) return false;

    int64_t leaf_no = find_leaf(key);
    if (leaf_no == NULL_PAGE) return false;

    Page leaf;
    if (!read_page(leaf_no, leaf)) return false;

    int nk = leaf.num_keys();
    int pos = leaf.find_key_pos(nk, key);
    if (pos >= nk || leaf.key_at(pos) != key) return false;

    bool result = remove_entry(leaf_no, leaf, key);
    if (result) write_metadata();
    return result;
}

template <typename K, typename V>
bool BPlusTree<K, V>::remove_entry(int64_t page_no, Page& page, const K& key) {
    if (page.is_leaf()) {
        int nk = page.num_keys();
        int pos = page.find_key_pos(nk, key);
        if (pos >= nk || page.key_at(pos) != key) return false;

        page.remove_leaf_entry(pos, nk);
        page.set_num_keys(nk);
        write_page(page_no, page);

        if (page_no == m_root_page) {
            if (nk == 0) {
                m_root_page = NULL_PAGE;
            }
            return true;
        }

        if (nk >= MIN_LEAF_KEYS) return true;

        int64_t parent_no = page.parent_page();
        Page parent;
        if (!read_page(parent_no, parent)) return true;

        int pnk = parent.num_keys();
        int idx = 0;
        if (parent.first_child() == page_no) {
            idx = -1;
        } else {
            for (int i = 0; i < pnk; ++i) {
                if (parent.child_at(i) == page_no) { idx = i; break; }
            }
        }

        int64_t left_sib = NULL_PAGE;
        int64_t right_sib = NULL_PAGE;

        if (idx == -1) {
            if (pnk > 0) right_sib = parent.child_at(0);
        } else if (idx == pnk - 1) {
            if (idx >= 0) left_sib = parent.child_at(idx - 1 >= 0 ? idx - 1 : 0);
            if (pnk == 0) left_sib = parent.first_child();
        } else {
            left_sib = parent.child_at(idx - 1 >= 0 ? idx - 1 : 0);
            if (idx == -1) left_sib = parent.first_child();
            right_sib = parent.child_at(idx + 1);
        }

        Page left_page, right_page;
        int left_nk = 0, right_nk = 0;

        if (left_sib != NULL_PAGE) {
            if (!read_page(left_sib, left_page)) left_sib = NULL_PAGE;
            else left_nk = left_page.num_keys();
        }
        if (right_sib != NULL_PAGE) {
            if (!read_page(right_sib, right_page)) right_sib = NULL_PAGE;
            else right_nk = right_page.num_keys();
        }

        if (left_sib != NULL_PAGE && left_nk > MIN_LEAF_KEYS) {
            return redistribute_nodes(page_no, page, left_sib, left_page, idx, parent_no, true);
        }
        if (right_sib != NULL_PAGE && right_nk > MIN_LEAF_KEYS) {
            return redistribute_nodes(page_no, page, right_sib, right_page, idx, parent_no, false);
        }
        if (left_sib != NULL_PAGE) {
            K mid = (idx >= 0 && idx < pnk) ? parent.internal_key_at(idx) : parent.internal_key_at(0);
            return coalesce_nodes(page_no, page, left_sib, left_page, idx, parent_no, mid);
        }
        if (right_sib != NULL_PAGE) {
            K mid = (idx >= 0 && idx + 1 < pnk) ? parent.internal_key_at(idx + 1) : parent.internal_key_at(0);
            return coalesce_nodes(page_no, page, right_sib, right_page, idx + 1, parent_no, mid);
        }
        return true;
    }

    int pnk = page.num_keys();
    int pos = page.find_internal_key_pos(pnk, key);

    int64_t child_page;
    if (pos < pnk && page.internal_key_at(pos) == key) {
        child_page = page.child_at(pos);
    } else {
        if (pos == 0) child_page = page.first_child();
        else child_page = page.child_at(pos - 1);
    }

    Page child;
    if (!read_page(child_page, child)) return false;

    if (!remove_entry(child_page, child, key)) return false;

    Page child_after;
    if (!read_page(child_page, child_after)) return true;

    if (!child_after.is_leaf()) {
        int cnk = child_after.num_keys();
        if (cnk < MIN_INTERNAL_KEYS) {
            int64_t parent_no = child_after.parent_page();
            if (parent_no == NULL_PAGE) {
                if (cnk == 0) {
                    m_root_page = child_after.first_child();
                    if (m_root_page != NULL_PAGE) {
                        Page new_root;
                        if (read_page(m_root_page, new_root)) {
                            new_root.set_parent_page(NULL_PAGE);
                            write_page(m_root_page, new_root);
                        }
                    }
                }
                return true;
            }

            Page parent;
            if (!read_page(parent_no, parent)) return true;

            int ppnk = parent.num_keys();
            int idx = -1;
            if (parent.first_child() == child_page) idx = -1;
            else {
                for (int i = 0; i < ppnk; ++i) {
                    if (parent.child_at(i) == child_page) { idx = i; break; }
                }
            }

            int64_t left_sib = NULL_PAGE, right_sib = NULL_PAGE;
            if (idx == -1) {
                if (ppnk > 0) right_sib = parent.child_at(0);
            } else if (idx == ppnk - 1) {
                left_sib = parent.child_at(idx - 1 >= 0 ? idx - 1 : 0);
                if (ppnk == 0) left_sib = parent.first_child();
            } else {
                left_sib = parent.child_at(idx - 1 >= 0 ? idx - 1 : 0);
                if (idx == -1) left_sib = parent.first_child();
                right_sib = parent.child_at(idx + 1);
            }

            Page lsib_p, rsib_p;
            int l_nk = 0, r_nk = 0;
            if (left_sib != NULL_PAGE && read_page(left_sib, lsib_p)) l_nk = lsib_p.num_keys();
            if (right_sib != NULL_PAGE && read_page(right_sib, rsib_p)) r_nk = rsib_p.num_keys();

            if (left_sib != NULL_PAGE && l_nk > MIN_INTERNAL_KEYS) {
                return redistribute_nodes(child_page, child_after, left_sib, lsib_p, idx, parent_no, true);
            }
            if (right_sib != NULL_PAGE && r_nk > MIN_INTERNAL_KEYS) {
                return redistribute_nodes(child_page, child_after, right_sib, rsib_p, idx, parent_no, false);
            }
            if (left_sib != NULL_PAGE) {
                K mid = (idx >= 0 && idx < ppnk) ? parent.internal_key_at(idx) : parent.internal_key_at(0);
                return coalesce_nodes(child_page, child_after, left_sib, lsib_p, idx, parent_no, mid);
            }
            if (right_sib != NULL_PAGE) {
                K mid = (idx >= 0 && idx + 1 < ppnk) ? parent.internal_key_at(idx + 1) : parent.internal_key_at(0);
                return coalesce_nodes(child_page, child_after, right_sib, rsib_p, idx + 1, parent_no, mid);
            }
        }
    } else {
        int cnk = child_after.num_keys();
        if (cnk < MIN_LEAF_KEYS && child_page != m_root_page) {
            int64_t parent_no = child_after.parent_page();
            Page parent;
            if (!read_page(parent_no, parent)) return true;

            int ppnk = parent.num_keys();
            int idx = -1;
            if (parent.first_child() == child_page) idx = -1;
            else {
                for (int i = 0; i < ppnk; ++i) {
                    if (parent.child_at(i) == child_page) { idx = i; break; }
                }
            }

            int64_t left_sib = NULL_PAGE, right_sib = NULL_PAGE;
            if (idx == -1) {
                if (ppnk > 0) right_sib = parent.child_at(0);
            } else if (idx == ppnk - 1) {
                left_sib = parent.child_at(idx - 1 >= 0 ? idx - 1 : 0);
                if (ppnk == 0) left_sib = parent.first_child();
            } else {
                left_sib = parent.child_at(idx - 1 >= 0 ? idx - 1 : 0);
                if (idx == -1) left_sib = parent.first_child();
                right_sib = parent.child_at(idx + 1);
            }

            Page lsib_p, rsib_p;
            int l_nk = 0, r_nk = 0;
            if (left_sib != NULL_PAGE && read_page(left_sib, lsib_p)) l_nk = lsib_p.num_keys();
            if (right_sib != NULL_PAGE && read_page(right_sib, rsib_p)) r_nk = rsib_p.num_keys();

            if (left_sib != NULL_PAGE && l_nk > MIN_LEAF_KEYS) {
                return redistribute_nodes(child_page, child_after, left_sib, lsib_p, idx, parent_no, true);
            }
            if (right_sib != NULL_PAGE && r_nk > MIN_LEAF_KEYS) {
                return redistribute_nodes(child_page, child_after, right_sib, rsib_p, idx, parent_no, false);
            }
            if (left_sib != NULL_PAGE) {
                K mid = (idx >= 0 && idx < ppnk) ? parent.internal_key_at(idx) : parent.internal_key_at(0);
                return coalesce_nodes(child_page, child_after, left_sib, lsib_p, idx, parent_no, mid);
            }
            if (right_sib != NULL_PAGE) {
                K mid = (idx >= 0 && idx + 1 < ppnk) ? parent.internal_key_at(idx + 1) : parent.internal_key_at(0);
                return coalesce_nodes(child_page, child_after, right_sib, rsib_p, idx + 1, parent_no, mid);
            }
        }
    }
    return true;
}

template <typename K, typename V>
bool BPlusTree<K, V>::coalesce_nodes(int64_t page_no, Page& page, int64_t neighbor_no, Page& neighbor,
                                      int parent_idx, int64_t parent_no, const K& middle_key) {
    Page parent;
    if (!read_page(parent_no, parent)) return false;
    int ppnk = parent.num_keys();

    if (page.is_leaf()) {
        int nk = page.num_keys();
        int nnk = neighbor.num_keys();

        for (int i = 0; i < nk; ++i) {
            neighbor.set_key_at(nnk + i, page.key_at(i));
            neighbor.set_value_at(nnk + i, page.value_at(i));
        }
        neighbor.set_num_keys(nnk + nk);
        neighbor.set_next_leaf(page.next_leaf());
        write_page(neighbor_no, neighbor);

        if (parent_idx == -1) {
            if (ppnk > 0) {
                parent.set_first_child(parent.child_at(0));
                parent.remove_internal_entry(0, ppnk);
                parent.set_num_keys(ppnk);
            } else {
                parent.set_first_child(neighbor_no);
            }
        } else if (parent_idx < ppnk) {
            parent.remove_internal_entry(parent_idx, ppnk);
            parent.set_num_keys(ppnk);
        }
        write_page(parent_no, parent);

        if (parent_no == page.parent_page()) {
            neighbor.set_parent_page(parent_no);
            write_page(neighbor_no, neighbor);
        }
    } else {
        int nk = page.num_keys();
        int nnk = neighbor.num_keys();

        neighbor.set_internal_key_at(nnk, middle_key);
        neighbor.set_child_at(nnk, page.first_child());
        ++nnk;
        for (int i = 0; i < nk; ++i) {
            neighbor.set_internal_key_at(nnk + i, page.internal_key_at(i));
            neighbor.set_child_at(nnk + i, page.child_at(i));
        }
        neighbor.set_num_keys(nnk + nk);
        write_page(neighbor_no, neighbor);

        update_parent_pointers(page.first_child(), neighbor_no);
        for (int i = 0; i < nk; ++i) {
            update_parent_pointers(page.child_at(i), neighbor_no);
        }

        if (parent_idx == -1) {
            if (ppnk > 0) {
                parent.set_first_child(parent.child_at(0));
                parent.remove_internal_entry(0, ppnk);
                parent.set_num_keys(ppnk);
            } else {
                parent.set_first_child(neighbor_no);
            }
        } else if (parent_idx < ppnk) {
            parent.remove_internal_entry(parent_idx, ppnk);
            parent.set_num_keys(ppnk);
        }
        write_page(parent_no, parent);

        if (parent_no == m_root_page && parent.num_keys() == 0) {
            m_root_page = neighbor_no;
            neighbor.set_parent_page(NULL_PAGE);
            write_page(neighbor_no, neighbor);
        }
    }

    return true;
}

template <typename K, typename V>
bool BPlusTree<K, V>::redistribute_nodes(int64_t page_no, Page& page, int64_t neighbor_no, Page& neighbor,
                                          int parent_idx, int64_t parent_no, bool from_left) {
    Page parent;
    if (!read_page(parent_no, parent)) return false;
    int ppnk = parent.num_keys();

    if (page.is_leaf()) {
        int nk = page.num_keys();
        int nnk = neighbor.num_keys();

        if (from_left) {
            K borrowed_key = neighbor.key_at(nnk - 1);
            V borrowed_val = neighbor.value_at(nnk - 1);
            neighbor.remove_leaf_entry(nnk - 1, nnk);
            neighbor.set_num_keys(nnk);

            page.insert_leaf_entry(0, borrowed_key, borrowed_val, nk);
            page.set_num_keys(nk);

            if (parent_idx >= 0 && parent_idx < ppnk) {
                parent.set_internal_key_at(parent_idx, page.key_at(0));
            }
        } else {
            K borrowed_key = neighbor.key_at(0);
            V borrowed_val = neighbor.value_at(0);
            neighbor.remove_leaf_entry(0, nnk);
            neighbor.set_num_keys(nnk);

            page.insert_leaf_entry(nk, borrowed_key, borrowed_val, nk);
            page.set_num_keys(nk);

            if (parent_idx >= 0 && parent_idx < ppnk) {
                parent.set_internal_key_at(parent_idx, neighbor.key_at(0));
            }
        }
        write_page(neighbor_no, neighbor);
        write_page(page_no, page);
        write_page(parent_no, parent);
    } else {
        int nk = page.num_keys();
        int nnk = neighbor.num_keys();

        if (from_left) {
            K up_key = neighbor.internal_key_at(nnk - 1);
            int64_t child = neighbor.child_at(nnk - 1);
            int parent_key_pos = (parent_idx >= 0 && parent_idx < ppnk) ? parent_idx : 0;

            K down_key;
            if (parent_key_pos < ppnk) {
                down_key = parent.internal_key_at(parent_key_pos);
            } else {
                down_key = up_key;
            }

            parent.set_internal_key_at(parent_key_pos, up_key);

            page.set_first_child(child);
            page.insert_internal_entry(0, down_key, page.first_child(), nk);
            page.set_child_at(0, page.first_child());
            page.set_first_child(child);
            page.set_num_keys(nk);

            neighbor.remove_internal_entry(nnk - 1, nnk);
            neighbor.set_num_keys(nnk);

            update_parent_pointers(child, page_no);
        } else {
            K up_key = neighbor.internal_key_at(0);
            int64_t child = neighbor.first_child();
            int parent_key_pos = (parent_idx >= 0 && parent_idx + 1 < ppnk) ? (parent_idx + 1) : 0;

            K down_key;
            if (parent_key_pos < ppnk && parent_key_pos >= 0) {
                down_key = parent.internal_key_at(parent_key_pos);
            } else {
                down_key = up_key;
            }

            parent.set_internal_key_at(parent_key_pos, up_key);

            neighbor.set_first_child(neighbor.child_at(0));
            neighbor.remove_internal_entry(0, nnk);
            neighbor.set_num_keys(nnk);

            page.insert_internal_entry(nk, down_key, child, nk);
            page.set_num_keys(nk);

            update_parent_pointers(child, page_no);
        }
        write_page(neighbor_no, neighbor);
        write_page(page_no, page);
        write_page(parent_no, parent);
    }

    return true;
}
