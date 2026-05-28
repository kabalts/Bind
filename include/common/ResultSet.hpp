#pragma once

#include "common/String.hpp"
#include "common/ArrayList.hpp"
#include "common/Json.hpp"
#include <cstddef>

enum class ColumnType {
    Int,
    Double,
    String,
    Bool,
    Null
};

class ResultSet {
public:
    ResultSet();
    ResultSet(const ResultSet& other);
    ResultSet(ResultSet&& other) noexcept;
    ~ResultSet();

    ResultSet& operator=(const ResultSet& other);
    ResultSet& operator=(ResultSet&& other) noexcept;

    void add_column(const String& name, ColumnType type);

    size_t column_count() const;
    size_t row_count()    const;

    const String&     column_name(size_t index) const;
    ColumnType         column_type(size_t index) const;
    const ArrayList<String>& column_names() const;
    const ArrayList<ColumnType>& column_types() const;

    void add_row(const ArrayList<Json>& row);
    void add_row(ArrayList<Json>&& row);

    const ArrayList<Json>& get_row(size_t index) const;
    ArrayList<Json>&       get_row_ref(size_t index);

    const Json& get_value(size_t row, size_t col) const;
    Json&       get_value_ref(size_t row, size_t col);

    const Json& get_value(size_t row, const String& col_name) const;
    Json&       get_value_ref(size_t row, const String& col_name);

    int64_t col_index(const String& name) const;

    void clear();
    void swap(ResultSet& other) noexcept;

private:
    ArrayList<String>     m_column_names;
    ArrayList<ColumnType> m_column_types;
    ArrayList<ArrayList<Json>> m_rows;
};

void swap(ResultSet& a, ResultSet& b) noexcept;
