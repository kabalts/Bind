#pragma once

#include <cstddef>
#include <cstring>
#include <ostream>
#include <new>

class String {
public:
    static constexpr size_t npos = static_cast<size_t>(-1);

    String();
    String(const char* s);
    String(const char* s, size_t len);
    String(const String& other);
    String(String&& other) noexcept;
    ~String();

    String& operator=(const String& other);
    String& operator=(String&& other) noexcept;
    String& operator=(const char* s);

    String  operator+(const String& other) const;
    String  operator+(const char* s) const;
    friend String operator+(const char* lhs, const String& rhs);

    String& operator+=(const String& other);
    String& operator+=(const char* s);
    String& operator+=(char c);

    bool operator==(const String& other) const;
    bool operator==(const char* s) const;
    friend bool operator==(const char* lhs, const String& rhs);

    bool operator!=(const String& other) const;
    bool operator!=(const char* s) const;
    friend bool operator!=(const char* lhs, const String& rhs);

    bool operator<(const String& other) const;
    bool operator>(const String& other) const;
    bool operator<=(const String& other) const;
    bool operator>=(const String& other) const;

    char&       operator[](size_t index);
    const char& operator[](size_t index) const;

    size_t length() const;
    size_t size() const;
    bool   empty() const;
    const char* c_str() const;
    char*       data();
    const char* data() const;

    String substr(size_t pos, size_t count = npos) const;
    size_t find(const String& str, size_t pos = 0) const;
    size_t find(const char* s, size_t pos = 0) const;
    size_t find(char c, size_t pos = 0) const;

    void clear();
    void reserve(size_t new_cap);
    void swap(String& other) noexcept;

    friend std::ostream& operator<<(std::ostream& os, const String& s);

private:
    char*  m_data;
    size_t m_length;
    size_t m_capacity;

    static constexpr size_t SSO_CAPACITY = 15;

    void ensure_capacity(size_t needed);
    void init_from_cstr(const char* s, size_t len);
};

void swap(String& a, String& b) noexcept;
