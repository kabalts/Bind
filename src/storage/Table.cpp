#include "storage/Table.hpp"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cerrno>
#include <cstdlib>

#include <direct.h>
#define mkdir_single(path, mode) _mkdir(path)

static bool mkdir_recursive(const char* path) {
    char tmp[512];
    std::strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    for (char* p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir_single(tmp, 0755);
            *p = '/';
        }
    }
    int rc = mkdir_single(tmp, 0755);
    return (rc == 0 || errno == EEXIST);
}

Table::Table() : m_name(), m_columns(), m_data_file(), m_index(), m_data_dir() {}

Table::~Table() {
    close();
}

Table::Table(Table&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_columns(std::move(other.m_columns))
    , m_data_file(std::move(other.m_data_file))
    , m_index(std::move(other.m_index))
    , m_data_dir(std::move(other.m_data_dir)) {}

Table& Table::operator=(Table&& other) noexcept {
    if (this != &other) {
        close();
        m_name = std::move(other.m_name);
        m_columns = std::move(other.m_columns);
        m_data_file = std::move(other.m_data_file);
        m_index = std::move(other.m_index);
        m_data_dir = std::move(other.m_data_dir);
    }
    return *this;
}

bool Table::create(const String& name, const ArrayList<Column>& columns,
                   const String& data_dir) {
    close();
    m_name = name;
    m_columns = columns;
    m_data_dir = data_dir;

    String dir_path = data_dir + "/" + name;
    if (!mkdir_recursive(dir_path.c_str())) {
        return false;
    }

    std::remove(data_file_path().c_str());
    std::remove(index_file_path().c_str());

    if (!m_data_file.open(data_file_path().c_str())) {
        return false;
    }
    if (!m_index.open(index_file_path().c_str())) {
        m_data_file.close();
        return false;
    }
    save_meta();
    return true;
}

bool Table::open(const String& name, const String& data_dir) {
    close();
    m_name = name;
    m_data_dir = data_dir;

    if (!m_data_file.open(data_file_path().c_str())) {
        return false;
    }
    if (!m_index.open(index_file_path().c_str())) {
        m_data_file.close();
        return false;
    }
    return true;
}

void Table::close() {
    m_data_file.close();
    m_index.close();
}

bool Table::is_open() const {
    return m_data_file.is_open() && m_index.is_open();
}

String Table::data_file_path() const {
    return m_data_dir + "/" + m_name + "/" + m_name + ".dat";
}

String Table::index_file_path() const {
    return m_data_dir + "/" + m_name + "/" + m_name + ".idx";
}

int32_t Table::extract_pk(const Json& row_data) const {
    if (m_columns.size() == 0) return 0;
    const String& pk_name = m_columns[0].name;
    if (row_data.is_object() && row_data.has_key(pk_name)) {
        const Json& val = row_data[pk_name];
        if (val.is_number()) {
            return static_cast<int32_t>(val.as_number());
        }
    }
    return 0;
}

int32_t Table::extract_pk_from_record(const FileEngine::Record& rec) const {
    return rec.id;
}

FileEngine::Record Table::json_to_record(const Json& row_data) const {
    FileEngine::Record rec;
    std::memset(&rec, 0, sizeof(rec));
    rec.id = extract_pk(row_data);
    rec.deleted = false;

    if (m_columns.size() >= 2 && row_data.is_object()) {
        const String& data_col = m_columns[1].name;
        if (row_data.has_key(data_col)) {
            const Json& val = row_data[data_col];
            if (val.is_string()) {
                const String& s = val.as_string();
                size_t copy_len = s.length();
                if (copy_len >= FileEngine::STRING_FIELD_SIZE) {
                    copy_len = FileEngine::STRING_FIELD_SIZE - 1;
                }
                std::memcpy(rec.data, s.c_str(), copy_len);
                rec.data[copy_len] = '\0';
            } else if (val.is_number()) {
                int n = std::snprintf(rec.data, FileEngine::STRING_FIELD_SIZE,
                                      "%.17g", val.as_number());
                if (n < 0) rec.data[0] = '\0';
                else if (static_cast<size_t>(n) >= FileEngine::STRING_FIELD_SIZE) {
                    rec.data[FileEngine::STRING_FIELD_SIZE - 1] = '\0';
                }
            } else if (val.is_bool()) {
                const char* s = val.as_bool() ? "true" : "false";
                std::snprintf(rec.data, FileEngine::STRING_FIELD_SIZE, "%s", s);
            } else {
                rec.data[0] = '\0';
            }
        }
    }
    return rec;
}

Json Table::record_to_json(const FileEngine::Record& rec) const {
    Json row = Json::object();
    if (m_columns.size() >= 1) {
        row.insert(m_columns[0].name, Json(static_cast<int>(rec.id)));
    }
    if (m_columns.size() >= 2) {
        row.insert(m_columns[1].name, Json(rec.data));
    }
    return row;
}

bool Table::evaluate_condition(const Json& row_json, const Condition& cond) const {
    if (!row_json.has_key(cond.column_name)) return false;

    const Json& cell = row_json[cond.column_name];
    const Json& rhs  = cond.value;

    if (cell.is_number() && rhs.is_number()) {
        double lhs_val = cell.as_number();
        double rhs_val = rhs.as_number();
        switch (cond.op) {
        case ConditionOp::EQ: return lhs_val == rhs_val;
        case ConditionOp::NE: return lhs_val != rhs_val;
        case ConditionOp::LT: return lhs_val <  rhs_val;
        case ConditionOp::LE: return lhs_val <= rhs_val;
        case ConditionOp::GT: return lhs_val >  rhs_val;
        case ConditionOp::GE: return lhs_val >= rhs_val;
        }
    } else if (cell.is_string() && rhs.is_string()) {
        const String& lhs_val = cell.as_string();
        const String& rhs_val = rhs.as_string();
        int cmp = 0;
        if (lhs_val < rhs_val) cmp = -1;
        else if (rhs_val < lhs_val) cmp = 1;

        switch (cond.op) {
        case ConditionOp::EQ: return cmp == 0;
        case ConditionOp::NE: return cmp != 0;
        case ConditionOp::LT: return cmp <  0;
        case ConditionOp::LE: return cmp <= 0;
        case ConditionOp::GT: return cmp >  0;
        case ConditionOp::GE: return cmp >= 0;
        }
    } else if (cell.is_bool() && rhs.is_bool()) {
        bool lhs_val = cell.as_bool();
        bool rhs_val = rhs.as_bool();
        switch (cond.op) {
        case ConditionOp::EQ: return lhs_val == rhs_val;
        case ConditionOp::NE: return lhs_val != rhs_val;
        default: return false;
        }
    }

    return false;
}

bool Table::insert(const Json& row_data) {
    if (!is_open()) return false;

    int32_t pk = extract_pk(row_data);

    int64_t dummy;
    if (m_index.search(pk, dummy)) {
        return false;
    }

    FileEngine::Record rec = json_to_record(row_data);

    int64_t rec_index = m_data_file.append(rec);
    if (rec_index < 0) return false;

    int64_t offset = rec_index * FileEngine::RECORD_SIZE;

    if (!m_index.insert(pk, offset)) {
        m_data_file.delete_at(offset);
        return false;
    }
    return true;
}

bool Table::remove(const Condition& cond) {
    if (!is_open()) return false;

    bool has_pk_cond = false;
    int32_t pk_value = 0;

    if (m_columns.size() > 0 && cond.column_name == m_columns[0].name &&
        cond.op == ConditionOp::EQ && cond.value.is_number()) {
        has_pk_cond = true;
        pk_value = static_cast<int32_t>(cond.value.as_number());
    }

    if (has_pk_cond) {
        int64_t offset = -1;
        if (!m_index.search(pk_value, offset)) return false;
        m_index.remove(pk_value);
        return m_data_file.delete_at(offset);
    }

    bool any_removed = false;
    int64_t count = m_data_file.record_count();
    for (int64_t i = 0; i < count; ++i) {
        FileEngine::Record rec;
        if (!m_data_file.read_record(i, rec)) continue;
        if (rec.deleted) continue;

        Json row_json = record_to_json(rec);
        if (evaluate_condition(row_json, cond)) {
            int64_t offset = i * FileEngine::RECORD_SIZE;
            m_index.remove(rec.id);
            m_data_file.delete_at(offset);
            any_removed = true;
        }
    }
    return any_removed;
}

bool Table::update(const Condition& cond, const Json& new_values) {
    if (!is_open()) return false;

    bool any_updated = false;
    int64_t count = m_data_file.record_count();

    for (int64_t i = 0; i < count; ++i) {
        FileEngine::Record rec;
        if (!m_data_file.read_record(i, rec)) continue;
        if (rec.deleted) continue;

        Json row_json = record_to_json(rec);
        if (!evaluate_condition(row_json, cond)) continue;

        if (new_values.is_object()) {
            if (new_values.has_key(m_columns[1].name)) {
                const Json& v = new_values[m_columns[1].name];
                if (v.is_string()) {
                    const String& s = v.as_string();
                    size_t copy_len = s.length();
                    if (copy_len >= FileEngine::STRING_FIELD_SIZE) {
                        copy_len = FileEngine::STRING_FIELD_SIZE - 1;
                    }
                    std::memcpy(rec.data, s.c_str(), copy_len);
                    rec.data[copy_len] = '\0';
                }
            }
        }

        int64_t offset = i * FileEngine::RECORD_SIZE;
        m_data_file.update_at(offset, rec);
        any_updated = true;
    }
    return any_updated;
}

ResultSet Table::select(const ArrayList<String>& col_names,
                        const Condition* cond) const {
    ResultSet rs;

    for (size_t i = 0; i < col_names.size(); ++i) {
        const String& cn = col_names[i];
        ColumnType ct = ColumnType::Null;
        for (size_t j = 0; j < m_columns.size(); ++j) {
            if (m_columns[j].name == cn) {
                ct = m_columns[j].type;
                break;
            }
        }
        rs.add_column(cn, ct);
    }

    int64_t count = m_data_file.record_count();
    for (int64_t i = 0; i < count; ++i) {
        FileEngine::Record rec;
        if (!m_data_file.read_record(i, rec)) continue;
        if (rec.deleted) continue;

        Json row_json = record_to_json(rec);

        if (cond && !evaluate_condition(row_json, *cond)) continue;

        ArrayList<Json> row;
        for (size_t j = 0; j < col_names.size(); ++j) {
            const String& cn = col_names[j];
            if (row_json.has_key(cn)) {
                row.push_back(row_json[cn]);
            } else {
                row.push_back(Json());
            }
        }
        rs.add_row(std::move(row));
    }
    return rs;
}

ResultSet Table::select_all() const {
    ArrayList<String> all_cols;
    for (size_t i = 0; i < m_columns.size(); ++i) {
        all_cols.push_back(m_columns[i].name);
    }
    return select(all_cols, nullptr);
}

String Table::meta_file_path() const {
    return m_data_dir + "/" + m_name + "/" + m_name + ".meta";
}

bool Table::save_meta() const {
    FILE* f = std::fopen(meta_file_path().c_str(), "w");
    if (!f) return false;
    for (size_t i = 0; i < m_columns.size(); ++i) {
        const char* type_str = "Null";
        switch (m_columns[i].type) {
        case ColumnType::Int:    type_str = "Int";    break;
        case ColumnType::Double: type_str = "Double"; break;
        case ColumnType::String: type_str = "String"; break;
        case ColumnType::Bool:   type_str = "Bool";   break;
        default: break;
        }
        std::fprintf(f, "%s %s\n", m_columns[i].name.c_str(), type_str);
    }
    std::fclose(f);
    return true;
}

bool Table::load_meta() {
    FILE* f = std::fopen(meta_file_path().c_str(), "r");
    if (!f) return false;
    m_columns.clear();
    char name_buf[128];
    char type_buf[32];
    while (std::fscanf(f, "%127s %31s", name_buf, type_buf) == 2) {
        ColumnType ct = ColumnType::Null;
        if (std::strcmp(type_buf, "Int") == 0) ct = ColumnType::Int;
        else if (std::strcmp(type_buf, "Double") == 0) ct = ColumnType::Double;
        else if (std::strcmp(type_buf, "String") == 0) ct = ColumnType::String;
        else if (std::strcmp(type_buf, "Bool") == 0) ct = ColumnType::Bool;
        m_columns.push_back(Column(String(name_buf), ct));
    }
    std::fclose(f);
    return m_columns.size() > 0;
}

bool Table::open_existing(const String& name, const String& data_dir) {
    close();
    m_name = name;
    m_data_dir = data_dir;
    if (!load_meta()) return false;
    if (!m_data_file.open(data_file_path().c_str())) return false;
    if (!m_index.open(index_file_path().c_str())) {
        m_data_file.close();
        return false;
    }
    return true;
}

int64_t Table::row_count() const {
    int64_t count = 0;
    int64_t total = m_data_file.record_count();
    for (int64_t i = 0; i < total; ++i) {
        FileEngine::Record rec;
        if (m_data_file.read_record(i, rec) && !rec.deleted) {
            ++count;
        }
    }
    return count;
}
