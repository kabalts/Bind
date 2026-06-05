#include "common/String.hpp"

#include <cstring>
#include <algorithm>

String::String() : m_data(nullptr), m_length(0), m_capacity(0) {
    m_data = new char[1];
    m_data[0] = '\0';
    m_capacity = 1;
}

String::String(const char* s) : m_data(nullptr), m_length(0), m_capacity(0) {
    if (s) {
        init_from_cstr(s, std::strlen(s));
    } else {
        m_data = new char[1];
        m_data[0] = '\0';
        m_capacity = 1;
    }
}

String::String(const char* s, size_t len) : m_data(nullptr), m_length(0), m_capacity(0) {
    init_from_cstr(s, len);
}

String::String(const String& other) : m_data(nullptr), m_length(0), m_capacity(0) {
    init_from_cstr(other.m_data, other.m_length);
}

String::String(String&& other) noexcept
    : m_data(other.m_data), m_length(other.m_length), m_capacity(other.m_capacity) {
    other.m_data = new char[1];
    other.m_data[0] = '\0';
    other.m_length = 0;
    other.m_capacity = 1;
}

String::~String() {
    delete[] m_data;
}

String& String::operator=(const String& other) {
    if (this != &other) {
        String tmp(other);
        swap(tmp);
    }
    return *this;
}

String& String::operator=(String&& other) noexcept {
    if (this != &other) {
        delete[] m_data;
        m_data     = other.m_data;
        m_length   = other.m_length;
        m_capacity = other.m_capacity;
        other.m_data = new char[1];
        other.m_data[0] = '\0';
        other.m_length = 0;
        other.m_capacity = 1;
    }
    return *this;
}

String& String::operator=(const char* s) {
    size_t len = s ? std::strlen(s) : 0;
    ensure_capacity(len + 1);
    if (s && len > 0) {
        std::memcpy(m_data, s, len);
    }
    m_data[len] = '\0';
    m_length = len;
    return *this;
}

String String::operator+(const String& other) const {
    String result;
    result.m_length = m_length + other.m_length;
    result.ensure_capacity(result.m_length + 1);
    if (m_length > 0) {
        std::memcpy(result.m_data, m_data, m_length);
    }
    if (other.m_length > 0) {
        std::memcpy(result.m_data + m_length, other.m_data, other.m_length);
    }
    result.m_data[result.m_length] = '\0';
    return result;
}

String String::operator+(const char* s) const {
    size_t slen = s ? std::strlen(s) : 0;
    String result;
    result.m_length = m_length + slen;
    result.ensure_capacity(result.m_length + 1);
    if (m_length > 0) {
        std::memcpy(result.m_data, m_data, m_length);
    }
    if (slen > 0) {
        std::memcpy(result.m_data + m_length, s, slen);
    }
    result.m_data[result.m_length] = '\0';
    return result;
}

String operator+(const char* lhs, const String& rhs) {
    size_t llen = lhs ? std::strlen(lhs) : 0;
    String result;
    result.m_length = llen + rhs.m_length;
    result.ensure_capacity(result.m_length + 1);
    if (llen > 0) {
        std::memcpy(result.m_data, lhs, llen);
    }
    if (rhs.m_length > 0) {
        std::memcpy(result.m_data + llen, rhs.m_data, rhs.m_length);
    }
    result.m_data[result.m_length] = '\0';
    return result;
}

String& String::operator+=(const String& other) {
    return operator+=(other.m_data);
}

String& String::operator+=(const char* s) {
    size_t slen = s ? std::strlen(s) : 0;
    if (slen == 0) return *this;
    ensure_capacity(m_length + slen + 1);
    std::memcpy(m_data + m_length, s, slen);
    m_length += slen;
    m_data[m_length] = '\0';
    return *this;
}

String& String::operator+=(char c) {
    ensure_capacity(m_length + 2);
    m_data[m_length] = c;
    ++m_length;
    m_data[m_length] = '\0';
    return *this;
}

bool String::operator==(const String& other) const {
    if (m_length != other.m_length) return false;
    return std::memcmp(m_data, other.m_data, m_length) == 0;
}

bool String::operator==(const char* s) const {
    size_t slen = s ? std::strlen(s) : 0;
    if (m_length != slen) return false;
    if (m_length == 0) return true;
    return std::memcmp(m_data, s, m_length) == 0;
}

bool operator==(const char* lhs, const String& rhs) {
    return rhs == lhs;
}

bool String::operator!=(const String& other) const { return !(*this == other); }
bool String::operator!=(const char* s) const        { return !(*this == s); }
bool operator!=(const char* lhs, const String& rhs) { return !(rhs == lhs); }

bool String::operator<(const String& other) const {
    size_t min_len = m_length < other.m_length ? m_length : other.m_length;
    int cmp = std::memcmp(m_data, other.m_data, min_len);
    if (cmp != 0) return cmp < 0;
    return m_length < other.m_length;
}

bool String::operator>(const String& other) const  { return other < *this; }
bool String::operator<=(const String& other) const { return !(other < *this); }
bool String::operator>=(const String& other) const { return !(*this < other); }

char& String::operator[](size_t index) {
    return m_data[index];
}

const char& String::operator[](size_t index) const {
    return m_data[index];
}

size_t String::length() const { return m_length; }
size_t String::size()   const { return m_length; }
bool   String::empty()  const { return m_length == 0; }

const char* String::c_str() const { return m_data; }
char*       String::data()       { return m_data; }
const char* String::data() const { return m_data; }

String String::substr(size_t pos, size_t count) const {
    if (pos >= m_length) return String();
    size_t actual_count = count;
    if (actual_count == npos || pos + actual_count > m_length) {
        actual_count = m_length - pos;
    }
    return String(m_data + pos, actual_count);
}

size_t String::find(const String& str, size_t pos) const {
    return find(str.m_data, pos);
}

size_t String::find(const char* s, size_t pos) const {
    if (!s || s[0] == '\0') return 0;
    size_t slen = std::strlen(s);
    if (slen > m_length || pos > m_length - slen) return npos;

    for (size_t i = pos; i <= m_length - slen; ++i) {
        if (std::memcmp(m_data + i, s, slen) == 0) {
            return i;
        }
    }
    return npos;
}

size_t String::find(char c, size_t pos) const {
    for (size_t i = pos; i < m_length; ++i) {
        if (m_data[i] == c) return i;
    }
    return npos;
}

void String::clear() {
    m_length = 0;
    if (m_data) m_data[0] = '\0';
}

void String::reserve(size_t new_cap) {
    ensure_capacity(new_cap);
}

void String::swap(String& other) noexcept {
    char*  tmp_data     = m_data;
    size_t tmp_length   = m_length;
    size_t tmp_capacity = m_capacity;
    m_data     = other.m_data;
    m_length   = other.m_length;
    m_capacity = other.m_capacity;
    other.m_data     = tmp_data;
    other.m_length   = tmp_length;
    other.m_capacity = tmp_capacity;
}

std::ostream& operator<<(std::ostream& os, const String& s) {
    if (s.m_data && s.m_length > 0) {
        os.write(s.m_data, static_cast<std::streamsize>(s.m_length));
    }
    return os;
}

void swap(String& a, String& b) noexcept {
    a.swap(b);
}

void String::ensure_capacity(size_t needed) {
    if (m_capacity >= needed) return;
    size_t new_cap = m_capacity < 8 ? 8 : m_capacity;
    while (new_cap < needed) {
        new_cap *= 2;
    }
    char* new_data = new char[new_cap];
    if (m_data && m_length > 0) {
        std::memcpy(new_data, m_data, m_length);
    }
    delete[] m_data;
    m_data = new_data;
    m_capacity = new_cap;
}

void String::init_from_cstr(const char* s, size_t len) {
    if (s && len > 0) {
        m_capacity = len + 1;
        m_data = new char[m_capacity];
        std::memcpy(m_data, s, len);
        m_data[len] = '\0';
        m_length = len;
    } else {
        m_data = new char[1];
        m_data[0] = '\0';
        m_capacity = 1;
        m_length = 0;
    }
}
