#include "common/Json.hpp"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <new>
#include <cerrno>

Json::Json()
    : m_type(JsonType::Null), m_bool_val(false), m_number_val(0.0),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(std::nullptr_t)
    : m_type(JsonType::Null), m_bool_val(false), m_number_val(0.0),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(bool val)
    : m_type(JsonType::Bool), m_bool_val(val), m_number_val(0.0),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(int val)
    : m_type(JsonType::Number), m_bool_val(false), m_number_val(static_cast<double>(val)),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(long val)
    : m_type(JsonType::Number), m_bool_val(false), m_number_val(static_cast<double>(val)),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(long long val)
    : m_type(JsonType::Number), m_bool_val(false), m_number_val(static_cast<double>(val)),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(unsigned int val)
    : m_type(JsonType::Number), m_bool_val(false), m_number_val(static_cast<double>(val)),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(unsigned long val)
    : m_type(JsonType::Number), m_bool_val(false), m_number_val(static_cast<double>(val)),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(unsigned long long val)
    : m_type(JsonType::Number), m_bool_val(false), m_number_val(static_cast<double>(val)),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(double val)
    : m_type(JsonType::Number), m_bool_val(false), m_number_val(val),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(const char* val)
    : m_type(JsonType::String), m_bool_val(false), m_number_val(0.0),
      m_string_val(new String(val ? val : "")), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(const String& val)
    : m_type(JsonType::String), m_bool_val(false), m_number_val(0.0),
      m_string_val(new String(val)), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(String&& val)
    : m_type(JsonType::String), m_bool_val(false), m_number_val(0.0),
      m_string_val(new String(std::move(val))), m_array_val(nullptr), m_object_val(nullptr) {}

Json::Json(const Json& other)
    : m_type(JsonType::Null), m_bool_val(false), m_number_val(0.0),
      m_string_val(nullptr), m_array_val(nullptr), m_object_val(nullptr) {
    copy_from(other);
}

Json::Json(Json&& other) noexcept
    : m_type(other.m_type), m_bool_val(other.m_bool_val), m_number_val(other.m_number_val),
      m_string_val(other.m_string_val), m_array_val(other.m_array_val),
      m_object_val(other.m_object_val) {
    other.m_type = JsonType::Null;
    other.m_bool_val = false;
    other.m_number_val = 0.0;
    other.m_string_val = nullptr;
    other.m_array_val = nullptr;
    other.m_object_val = nullptr;
}

Json::~Json() {
    destroy_current();
}

Json& Json::operator=(const Json& other) {
    if (this != &other) {
        destroy_current();
        copy_from(other);
    }
    return *this;
}

Json& Json::operator=(Json&& other) noexcept {
    if (this != &other) {
        destroy_current();
        m_type       = other.m_type;
        m_bool_val   = other.m_bool_val;
        m_number_val = other.m_number_val;
        m_string_val = other.m_string_val;
        m_array_val  = other.m_array_val;
        m_object_val = other.m_object_val;
        other.m_type = JsonType::Null;
        other.m_bool_val = false;
        other.m_number_val = 0.0;
        other.m_string_val = nullptr;
        other.m_array_val = nullptr;
        other.m_object_val = nullptr;
    }
    return *this;
}

JsonType Json::type() const { return m_type; }

bool Json::is_null()   const { return m_type == JsonType::Null; }
bool Json::is_bool()   const { return m_type == JsonType::Bool; }
bool Json::is_number() const { return m_type == JsonType::Number; }
bool Json::is_string() const { return m_type == JsonType::String; }
bool Json::is_array()  const { return m_type == JsonType::Array; }
bool Json::is_object() const { return m_type == JsonType::Object; }

bool Json::as_bool() const { return m_bool_val; }

double Json::as_number() const { return m_number_val; }

const String& Json::as_string() const { return *m_string_val; }

const ArrayList<Json>& Json::as_array() const { return *m_array_val; }

const LinkedList<Json::KeyValue>& Json::as_object() const { return *m_object_val; }

String& Json::as_string_ref() { return *m_string_val; }

ArrayList<Json>& Json::as_array_ref() { return *m_array_val; }

LinkedList<Json::KeyValue>& Json::as_object_ref() { return *m_object_val; }

Json& Json::operator[](size_t index) {
    return (*m_array_val)[index];
}

const Json& Json::operator[](size_t index) const {
    return (*m_array_val)[index];
}

Json& Json::operator[](const String& key) {
    for (auto& kv : *m_object_val) {
        if (kv.key == key) return kv.value;
    }
    m_object_val->push_back(KeyValue(key, Json()));
    return m_object_val->back().value;
}

const Json& Json::operator[](const String& key) const {
    for (const auto& kv : *m_object_val) {
        if (kv.key == key) return kv.value;
    }
    static Json null_val;
    return null_val;
}

size_t Json::size() const {
    if (m_type == JsonType::Array)  return m_array_val->size();
    if (m_type == JsonType::Object) return m_object_val->size();
    return 0;
}

Json Json::array() {
    Json j;
    j.m_type = JsonType::Array;
    j.m_array_val = new ArrayList<Json>();
    return j;
}

Json Json::object() {
    Json j;
    j.m_type = JsonType::Object;
    j.m_object_val = new LinkedList<KeyValue>();
    return j;
}

void Json::push_back(const Json& value) {
    if (m_type != JsonType::Array) {
        destroy_current();
        m_type = JsonType::Array;
        m_array_val = new ArrayList<Json>();
    }
    m_array_val->push_back(value);
}

void Json::push_back(Json&& value) {
    if (m_type != JsonType::Array) {
        destroy_current();
        m_type = JsonType::Array;
        m_array_val = new ArrayList<Json>();
    }
    m_array_val->push_back(std::move(value));
}

void Json::insert(const String& key, const Json& value) {
    if (m_type != JsonType::Object) {
        destroy_current();
        m_type = JsonType::Object;
        m_object_val = new LinkedList<KeyValue>();
    }
    m_object_val->push_back(KeyValue(key, value));
}

void Json::insert(const String& key, Json&& value) {
    if (m_type != JsonType::Object) {
        destroy_current();
        m_type = JsonType::Object;
        m_object_val = new LinkedList<KeyValue>();
    }
    m_object_val->push_back(KeyValue(key, std::move(value)));
}

bool Json::has_key(const String& key) const {
    if (m_type != JsonType::Object) return false;
    for (const auto& kv : *m_object_val) {
        if (kv.key == key) return true;
    }
    return false;
}

void Json::remove(const String& key) {
    if (m_type != JsonType::Object) return;
    for (auto it = m_object_val->begin(); it != m_object_val->end(); ++it) {
        if (it->key == key) {
            m_object_val->erase(it);
            return;
        }
    }
}

String Json::serialize() const {
    String result;
    switch (m_type) {
    case JsonType::Null:
        result = "null";
        break;
    case JsonType::Bool:
        result = m_bool_val ? "true" : "false";
        break;
    case JsonType::Number: {
        if (std::floor(m_number_val) == m_number_val && std::isfinite(m_number_val)) {
            char buf[64];
            int written = std::snprintf(buf, sizeof(buf), "%.0f", m_number_val);
            result = String(buf, static_cast<size_t>(written));
        } else {
            char buf[128];
            int written = std::snprintf(buf, sizeof(buf), "%.17g", m_number_val);
            result = String(buf, static_cast<size_t>(written));
        }
        break;
    }
    case JsonType::String: {
        result = "\"";
        serialize_string(*m_string_val, result);
        result += "\"";
        break;
    }
    case JsonType::Array: {
        result = "[";
        for (size_t i = 0; i < m_array_val->size(); ++i) {
            if (i > 0) result += ",";
            result += (*m_array_val)[i].serialize();
        }
        result += "]";
        break;
    }
    case JsonType::Object: {
        result = "{";
        bool first = true;
        for (const auto& kv : *m_object_val) {
            if (!first) result += ",";
            first = false;
            result += "\"";
            serialize_string(kv.key, result);
            result += "\":";
            result += kv.value.serialize();
        }
        result += "}";
        break;
    }
    }
    return result;
}

void Json::serialize_to(std::ostream& os) const {
    switch (m_type) {
    case JsonType::Null:
        os << "null";
        break;
    case JsonType::Bool:
        os << (m_bool_val ? "true" : "false");
        break;
    case JsonType::Number:
        if (std::floor(m_number_val) == m_number_val && std::isfinite(m_number_val)) {
            os << static_cast<long long>(m_number_val);
        } else {
            os << m_number_val;
        }
        break;
    case JsonType::String:
        os << "\"" << *m_string_val << "\"";
        break;
    case JsonType::Array: {
        os << "[";
        for (size_t i = 0; i < m_array_val->size(); ++i) {
            if (i > 0) os << ",";
            (*m_array_val)[i].serialize_to(os);
        }
        os << "]";
        break;
    }
    case JsonType::Object: {
        os << "{";
        bool first = true;
        for (const auto& kv : *m_object_val) {
            if (!first) os << ",";
            first = false;
            os << "\"" << kv.key << "\":";
            kv.value.serialize_to(os);
        }
        os << "}";
        break;
    }
    }
}

Json Json::parse(const String& json_str) {
    return parse(json_str.c_str());
}

Json Json::parse(const char* json_str) {
    const char* p = json_str;
    parse_whitespace(p);
    Json result = parse_value(p);
    parse_whitespace(p);
    return result;
}

void Json::destroy_current() {
    switch (m_type) {
    case JsonType::String:
        delete m_string_val;
        m_string_val = nullptr;
        break;
    case JsonType::Array:
        delete m_array_val;
        m_array_val = nullptr;
        break;
    case JsonType::Object:
        delete m_object_val;
        m_object_val = nullptr;
        break;
    default:
        break;
    }
    m_type = JsonType::Null;
}

void Json::copy_from(const Json& other) {
    m_type       = other.m_type;
    m_bool_val   = other.m_bool_val;
    m_number_val = other.m_number_val;
    m_string_val = nullptr;
    m_array_val  = nullptr;
    m_object_val = nullptr;

    switch (other.m_type) {
    case JsonType::String:
        m_string_val = new String(*other.m_string_val);
        break;
    case JsonType::Array:
        m_array_val = new ArrayList<Json>(*other.m_array_val);
        break;
    case JsonType::Object:
        m_object_val = new LinkedList<KeyValue>(*other.m_object_val);
        break;
    default:
        break;
    }
}

void Json::serialize_string(const String& s, String& out) {
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                out += buf;
            } else {
                out += c;
            }
            break;
        }
    }
}

void Json::parse_whitespace(const char*& p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        ++p;
    }
}

Json Json::parse_value(const char*& p) {
    parse_whitespace(p);
    switch (*p) {
    case '{': return parse_object(p);
    case '[': return parse_array(p);
    case '"': return Json(parse_json_string(p));
    case 't': case 'f': case 'n': return parse_literal(p);
    default:
        if (*p == '-' || (*p >= '0' && *p <= '9')) {
            return parse_number(p);
        }
        return Json();
    }
}

Json Json::parse_object(const char*& p) {
    Json obj = Json::object();
    ++p;
    parse_whitespace(p);
    if (*p == '}') {
        ++p;
        return obj;
    }
    while (true) {
        parse_whitespace(p);
        String key = parse_json_string(p);
        parse_whitespace(p);
        if (*p != ':') return obj;
        ++p;
        Json value = parse_value(p);
        obj.insert(key, std::move(value));
        parse_whitespace(p);
        if (*p == '}') {
            ++p;
            break;
        }
        if (*p != ',') break;
        ++p;
    }
    return obj;
}

Json Json::parse_array(const char*& p) {
    Json arr = Json::array();
    ++p;
    parse_whitespace(p);
    if (*p == ']') {
        ++p;
        return arr;
    }
    while (true) {
        Json value = parse_value(p);
        arr.push_back(std::move(value));
        parse_whitespace(p);
        if (*p == ']') {
            ++p;
            break;
        }
        if (*p != ',') break;
        ++p;
    }
    return arr;
}

String Json::parse_json_string(const char*& p) {
    String result;
    if (*p != '"') return result;
    ++p;
    while (*p != '\0') {
        if (*p == '"') {
            ++p;
            return result;
        }
        if (*p == '\\') {
            ++p;
            switch (*p) {
            case '"':  result += '"';  ++p; break;
            case '\\': result += '\\'; ++p; break;
            case '/':  result += '/';  ++p; break;
            case 'b':  result += '\b'; ++p; break;
            case 'f':  result += '\f'; ++p; break;
            case 'n':  result += '\n'; ++p; break;
            case 'r':  result += '\r'; ++p; break;
            case 't':  result += '\t'; ++p; break;
            case 'u': {
                ++p;
                unsigned int codepoint = 0;
                for (int i = 0; i < 4; ++i, ++p) {
                    codepoint <<= 4;
                    char hc = *p;
                    if (hc >= '0' && hc <= '9')      codepoint |= (hc - '0');
                    else if (hc >= 'a' && hc <= 'f')  codepoint |= (hc - 'a' + 10);
                    else if (hc >= 'A' && hc <= 'F')  codepoint |= (hc - 'A' + 10);
                }
                if (codepoint < 0x80) {
                    result += static_cast<char>(codepoint);
                } else if (codepoint < 0x800) {
                    result += static_cast<char>(0xC0 | (codepoint >> 6));
                    result += static_cast<char>(0x80 | (codepoint & 0x3F));
                } else {
                    result += static_cast<char>(0xE0 | (codepoint >> 12));
                    result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (codepoint & 0x3F));
                }
                break;
            }
            default:
                result += *p;
                ++p;
                break;
            }
        } else {
            result += *p;
            ++p;
        }
    }
    return result;
}

Json Json::parse_number(const char*& p) {
    const char* start = p;
    if (*p == '-') ++p;
    while (*p >= '0' && *p <= '9') ++p;
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9') ++p;
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') ++p;
        while (*p >= '0' && *p <= '9') ++p;
    }
    size_t len = static_cast<size_t>(p - start);
    char* end = nullptr;
    double val = std::strtod(start, &end);
    return Json(val);
}

Json Json::parse_literal(const char*& p) {
    if (std::strncmp(p, "true", 4) == 0) {
        p += 4;
        return Json(true);
    }
    if (std::strncmp(p, "false", 5) == 0) {
        p += 5;
        return Json(false);
    }
    if (std::strncmp(p, "null", 4) == 0) {
        p += 4;
        return Json();
    }
    return Json();
}
