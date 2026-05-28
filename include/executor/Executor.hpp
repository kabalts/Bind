#pragma once

#include "common/String.hpp"
#include "common/ArrayList.hpp"
#include "common/ResultSet.hpp"
#include "parser/SQLParser.hpp"
#include "storage/Database.hpp"

class Executor {
public:
    explicit Executor(const String& base_dir);
    ~Executor();

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    ResultSet execute(const SQLStatement& stmt);
    ResultSet execute_sql(const String& sql);

    const String& error_message() const { return m_error; }
    Database*     current_db() const     { return m_current_db; }

private:
    String              m_base_dir;
    ArrayList<Database*> m_databases;
    Database*           m_current_db;
    String              m_error;

    void set_error(const String& msg);
    void clear_error();
    Database* find_database(const String& name) const;
    Database* open_database(const String& name);
    void close_all();

    ResultSet exec_create_database(const SQLStatement& stmt);
    ResultSet exec_drop_database(const SQLStatement& stmt);
    ResultSet exec_use(const SQLStatement& stmt);
    ResultSet exec_create_table(const SQLStatement& stmt);
    ResultSet exec_drop_table(const SQLStatement& stmt);
    ResultSet exec_select(const SQLStatement& stmt);
    ResultSet exec_insert(const SQLStatement& stmt);
    ResultSet exec_delete(const SQLStatement& stmt);
    ResultSet exec_update(const SQLStatement& stmt);

    ResultSet make_message_result(const String& msg) const;
    ResultSet make_error_result(const String& err) const;
};
