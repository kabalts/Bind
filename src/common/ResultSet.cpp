#include "common/ResultSet.hpp"
#include <stdexcept>
#include <cstdint>

ResultSet::ResultSet() = default;

ResultSet::ResultSet(const ResultSet& other)
    : m_column_names(other.m_column_names)
    , m_column_types(other.m_column_types)
    , m_rows(other.m_rows) {}

ResultSet::ResultSet(ResultSet&& other) noexcept
    : m_column_names(std::move(other.m_column_names))
    , m_column_types(std::move(other.m_column_types))
    , m_rows(std::move(other.m_rows)) {}

ResultSet::~ResultSet() = default;

ResultSet& ResultSet::operator=(const ResultSet& other) {
    if (this != &other) {
        ResultSet tmp(other);
        swap(tmp);
    }
    return *this;
}

ResultSet& ResultSet::operator=(ResultSet&& other) noexcept {
    if (this != &other) {
        m_column_names = std::move(other.m_column_names);
        m_column_types = std::move(other.m_column_types);
        m_rows         = std::move(other.m_rows);
    }
    return *this;
}

void ResultSet::add_column(const String& name, ColumnType type) {
    m_column_names.push_back(name);
    m_column_types.push_back(type);
}

size_t ResultSet::column_count() const {
    return m_column_names.size();
}

size_t ResultSet::row_count() const {
    return m_rows.size();
}

const String& ResultSet::column_name(size_t index) const {
    return m_column_names.at(index);
}

ColumnType ResultSet::column_type(size_t index) const {
    return m_column_types.at(index);
}

const ArrayList<String>& ResultSet::column_names() const {
    return m_column_names;
}

const ArrayList<ColumnType>& ResultSet::column_types() const {
    return m_column_types;
}

void ResultSet::add_row(const ArrayList<Json>& row) {
    m_rows.push_back(row);
}

void ResultSet::add_row(ArrayList<Json>&& row) {
    m_rows.push_back(std::move(row));
}

const ArrayList<Json>& ResultSet::get_row(size_t index) const {
    return m_rows.at(index);
}

ArrayList<Json>& ResultSet::get_row_ref(size_t index) {
    return m_rows.at(index);
}

const Json& ResultSet::get_value(size_t row, size_t col) const {
    return m_rows.at(row).at(col);
}

Json& ResultSet::get_value_ref(size_t row, size_t col) {
    return m_rows.at(row).at(col);
}

const Json& ResultSet::get_value(size_t row, const String& col_name) const {
    int64_t idx = col_index(col_name);
    if (idx < 0) {
        throw std::out_of_range("ResultSet::get_value: column name not found");
    }
    return m_rows.at(row).at(static_cast<size_t>(idx));
}

Json& ResultSet::get_value_ref(size_t row, const String& col_name) {
    int64_t idx = col_index(col_name);
    if (idx < 0) {
        throw std::out_of_range("ResultSet::get_value_ref: column name not found");
    }
    return m_rows.at(row).at(static_cast<size_t>(idx));
}

int64_t ResultSet::col_index(const String& name) const {
    for (size_t i = 0; i < m_column_names.size(); ++i) {
        if (m_column_names[i] == name) {
            return static_cast<int64_t>(i);
        }
    }
    return -1;
}

void ResultSet::clear() {
    m_column_names.clear();
    m_column_types.clear();
    m_rows.clear();
}

void ResultSet::swap(ResultSet& other) noexcept {
    m_column_names.swap(other.m_column_names);
    m_column_types.swap(other.m_column_types);
    m_rows.swap(other.m_rows);
}

void swap(ResultSet& a, ResultSet& b) noexcept {
    a.swap(b);
}
