#pragma once

#include "common/String.hpp"
#include "common/ArrayList.hpp"
#include "common/LinkedList.hpp"
#include <cstddef>
#include <ostream>

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class Json {
public:
    struct KeyValue;

    Json();
    explicit Json(std::nullptr_t);
    explicit Json(bool val);
    explicit Json(int val);
    explicit Json(long val);
    explicit Json(long long val);
    explicit Json(unsigned int val);
    explicit Json(unsigned long val);
    explicit Json(unsigned long long val);
    explicit Json(double val);
    explicit Json(const char* val);
    explicit Json(const String& val);
    Json(String&& val);

    Json(const Json& other);
    Json(Json&& other) noexcept;
    ~Json();

    Json& operator=(const Json& other);
    Json& operator=(Json&& other) noexcept;

    JsonType type() const;
    bool is_null()    const;
    bool is_bool()    const;
    bool is_number()  const;
    bool is_string()  const;
    bool is_array()   const;
    bool is_object()  const;

    bool   as_bool()   const;
    double as_number() const;
    const String& as_string() const;
    const ArrayList<Json>& as_array() const;
    const LinkedList<KeyValue>& as_object() const;

    String& as_string_ref();
    ArrayList<Json>& as_array_ref();
    LinkedList<KeyValue>& as_object_ref();

    Json& operator[](size_t index);
    const Json& operator[](size_t index) const;
    Json& operator[](const String& key);
    const Json& operator[](const String& key) const;

    size_t size() const;

    static Json array();
    static Json object();

    void push_back(const Json& value);
    void push_back(Json&& value);

    void insert(const String& key, const Json& value);
    void insert(const String& key, Json&& value);

    bool has_key(const String& key) const;
    void remove(const String& key);

    String serialize() const;
    void serialize_to(std::ostream& os) const;

    static Json parse(const String& json_str);
    static Json parse(const char* json_str);

    friend bool operator==(const Json& a, const Json& b);
    friend bool operator!=(const Json& a, const Json& b);
    friend std::ostream& operator<<(std::ostream& os, const Json& json);

private:
    JsonType m_type;
    bool     m_bool_val;
    double   m_number_val;

    String*               m_string_val;
    ArrayList<Json>*      m_array_val;
    LinkedList<KeyValue>* m_object_val;

    void destroy_current();
    void copy_from(const Json& other);

    static void serialize_string(const String& s, String& out);
    static void parse_whitespace(const char*& p);
    static Json  parse_value(const char*& p);
    static Json  parse_object(const char*& p);
    static Json  parse_array(const char*& p);
    static String parse_json_string(const char*& p);
    static Json  parse_number(const char*& p);
    static Json  parse_literal(const char*& p);
};

struct Json::KeyValue {
    String key;
    Json   value;

    KeyValue() = default;
    KeyValue(const String& k, const Json& v) : key(k), value(v) {}
    KeyValue(String&& k, Json&& v) : key(std::move(k)), value(std::move(v)) {}
    KeyValue(const KeyValue& other) : key(other.key), value(other.value) {}
    KeyValue(KeyValue&& other) noexcept
        : key(std::move(other.key)), value(std::move(other.value)) {}
    KeyValue& operator=(const KeyValue& other) {
        key = other.key; value = other.value; return *this;
    }
    KeyValue& operator=(KeyValue&& other) noexcept {
        key = std::move(other.key); value = std::move(other.value); return *this;
    }
};

inline bool operator==(const Json& a, const Json& b) {
    if (a.m_type != b.m_type) return false;
    switch (a.m_type) {
    case JsonType::Null:   return true;
    case JsonType::Bool:   return a.m_bool_val == b.m_bool_val;
    case JsonType::Number: return a.m_number_val == b.m_number_val;
    case JsonType::String: return *a.m_string_val == *b.m_string_val;
    case JsonType::Array:
        if (a.m_array_val->size() != b.m_array_val->size()) return false;
        for (size_t i = 0; i < a.m_array_val->size(); ++i) {
            if (!((*a.m_array_val)[i] == (*b.m_array_val)[i])) return false;
        }
        return true;
    case JsonType::Object:
        if (a.m_object_val->size() != b.m_object_val->size()) return false;
        for (const auto& kv_a : *a.m_object_val) {
            bool found = false;
            for (const auto& kv_b : *b.m_object_val) {
                if (kv_a.key == kv_b.key) {
                    if (!(kv_a.value == kv_b.value)) return false;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    return false;
}

inline bool operator!=(const Json& a, const Json& b) {
    return !(a == b);
}

inline std::ostream& operator<<(std::ostream& os, const Json& json) {
    json.serialize_to(os);
    return os;
}
