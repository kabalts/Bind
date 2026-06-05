#pragma once

#include "common/String.hpp"
#include "common/ArrayList.hpp"
#include "common/Json.hpp"
#include "common/ResultSet.hpp"
#include "storage/FileEngine.hpp"
#include "storage/BPlusTree.hpp"
#include <cstdint>

enum class ConditionOp {
    EQ,  // ==
    NE,  // !=
    LT,  // <
    LE,  // <=
    GT,  // >
    GE   // >=
};

struct Column {
    String     name;
    ColumnType type;

    Column() : name(), type(ColumnType::Null) {}
    Column(const String& n, ColumnType t) : name(n), type(t) {}
};

struct Condition {
    String      column_name;
    ConditionOp op;
    Json        value;
};

class Table {
public:
    Table();
    ~Table();

    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&& other) noexcept;
    Table& operator=(Table&& other) noexcept;

    bool create(const String& name, const ArrayList<Column>& columns,
                const String& data_dir);
    bool open(const String& name, const String& data_dir);
    bool open_existing(const String& name, const String& data_dir);
    void close();
    bool is_open() const;

    const String& name() const { return m_name; }
    const ArrayList<Column>& columns() const { return m_columns; }

    bool insert(const Json& row_data);
    bool remove(const Condition& cond);
    bool update(const Condition& cond, const Json& new_values);
    ResultSet select(const ArrayList<String>& col_names,
                     const Condition* cond = nullptr) const;
    ResultSet select_all() const;

    int64_t row_count() const;

private:
    String              m_name;
    ArrayList<Column>   m_columns;
    FileEngine          m_data_file;
    BPlusTree<int32_t, int64_t> m_index;
    String              m_data_dir;

    int32_t extract_pk(const Json& row_data) const;
    int32_t extract_pk_from_record(const FileEngine::Record& rec) const;
    FileEngine::Record json_to_record(const Json& row_data) const;
    Json record_to_json(const FileEngine::Record& rec) const;
    bool evaluate_condition(const Json& row_json, const Condition& cond) const;

    String data_file_path() const;
    String index_file_path() const;
    String meta_file_path() const;
    bool save_meta() const;
    bool load_meta();
};
