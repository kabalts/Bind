#pragma once

#include "common/String.hpp"
#include "common/ArrayList.hpp"
#include "storage/Table.hpp"

class Database {
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool create(const String& name, const String& base_dir);
    bool open(const String& name, const String& base_dir);
    void close();
    bool is_open() const;

    const String& name() const { return m_name; }

    Table* create_table(const String& table_name,
                        const ArrayList<Column>& columns);
    bool    drop_table(const String& table_name);
    Table*  get_table(const String& table_name);
    const   ArrayList<Table>& tables() const { return m_tables; }

private:
    String            m_name;
    String            m_base_dir;
    ArrayList<Table>  m_tables;
    bool              m_open;

    String data_dir() const;
};
