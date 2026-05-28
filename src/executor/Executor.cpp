#include "executor/Executor.hpp"
#include <cstdio>

#ifdef _WIN32
#include <direct.h>
#define rmdir(path) _rmdir(path)
#else
#include <unistd.h>
#endif

Executor::Executor(const String& base_dir)
    : m_base_dir(base_dir), m_databases(), m_current_db(nullptr), m_error() {}

Executor::~Executor() {
    close_all();
}

void Executor::set_error(const String& msg) {
    m_error = msg;
}

void Executor::clear_error() {
    m_error = String();
}

void Executor::close_all() {
    for (size_t i = 0; i < m_databases.size(); ++i) {
        if (m_databases[i]) {
            m_databases[i]->close();
            delete m_databases[i];
        }
    }
    m_databases.clear();
    m_current_db = nullptr;
}

Database* Executor::find_database(const String& name) const {
    for (size_t i = 0; i < m_databases.size(); ++i) {
        if (m_databases[i] && m_databases[i]->name() == name) {
            return m_databases[i];
        }
    }
    return nullptr;
}

Database* Executor::open_database(const String& name) {
    Database* db = find_database(name);
    if (db) return db;

    Database* new_db = new Database();
    if (!new_db->open(name, m_base_dir)) {
        delete new_db;
        set_error(String("Failed to open database: ") + name);
        return nullptr;
    }
    m_databases.push_back(new_db);
    return new_db;
}

ResultSet Executor::execute(const SQLStatement& stmt) {
    clear_error();

    if (!stmt.error_msg.empty()) {
        set_error(stmt.error_msg);
        return make_error_result(stmt.error_msg);
    }

    switch (stmt.type) {
    case StmtType::CREATE_DATABASE: return exec_create_database(stmt);
    case StmtType::DROP_DATABASE:   return exec_drop_database(stmt);
    case StmtType::USE:             return exec_use(stmt);
    case StmtType::CREATE_TABLE:    return exec_create_table(stmt);
    case StmtType::DROP_TABLE:      return exec_drop_table(stmt);
    case StmtType::SELECT:          return exec_select(stmt);
    case StmtType::INSERT:          return exec_insert(stmt);
    case StmtType::DELETE:          return exec_delete(stmt);
    case StmtType::UPDATE:          return exec_update(stmt);
    case StmtType::EXIT:
        return make_message_result("bye");
    case StmtType::NONE:
    default:
        set_error("Unknown statement type");
        return make_error_result("Unknown statement type");
    }
}

ResultSet Executor::execute_sql(const String& sql) {
    SQLParser parser;
    SQLStatement stmt = parser.parse(sql);
    if (!stmt.error_msg.empty()) {
        set_error(stmt.error_msg);
        return make_error_result(stmt.error_msg);
    }
    if (stmt.type == StmtType::NONE) {
        return make_message_result("");
    }
    return execute(stmt);
}

ResultSet Executor::make_message_result(const String& msg) const {
    ResultSet rs;
    rs.add_column("message", ColumnType::String);
    ArrayList<Json> row;
    row.push_back(Json(msg));
    rs.add_row(std::move(row));
    return rs;
}

ResultSet Executor::make_error_result(const String& err) const {
    ResultSet rs;
    rs.add_column("error", ColumnType::String);
    ArrayList<Json> row;
    row.push_back(Json(err));
    rs.add_row(std::move(row));
    return rs;
}

ResultSet Executor::exec_create_database(const SQLStatement& stmt) {
    Database* new_db = new Database();
    if (!new_db->create(stmt.db_name, m_base_dir)) {
        delete new_db;
        set_error(String("Failed to create database: ") + stmt.db_name);
        return make_error_result(String("Failed to create database: ") + stmt.db_name);
    }
    m_databases.push_back(new_db);
    return make_message_result(String("Database '") + stmt.db_name + "' created.");
}

ResultSet Executor::exec_drop_database(const SQLStatement& stmt) {
    Database* db = find_database(stmt.db_name);
    if (!db) {
        db = open_database(stmt.db_name);
    }
    if (db) {
        while (db->tables().size() > 0) {
            db->drop_table(db->tables()[0].name());
        }
        db->close();
        for (size_t i = 0; i < m_databases.size(); ++i) {
            if (m_databases[i] == db) {
                delete m_databases[i];
                m_databases.erase(m_databases.cbegin() + static_cast<long>(i));
                break;
            }
        }
        if (m_current_db == db) {
            m_current_db = nullptr;
        }
    }

    String db_path = m_base_dir + "/" + stmt.db_name;
    rmdir(db_path.c_str());

    return make_message_result(String("Database '") + stmt.db_name + "' dropped.");
}

ResultSet Executor::exec_use(const SQLStatement& stmt) {
    Database* db = open_database(stmt.db_name);
    if (!db) {
        return make_error_result(String("Cannot use database: ") + m_error);
    }
    m_current_db = db;
    return make_message_result(String("Using database '") + stmt.db_name + "'.");
}

ResultSet Executor::exec_create_table(const SQLStatement& stmt) {
    if (!m_current_db) {
        set_error("No database selected");
        return make_error_result("No database selected. Use 'use <dbname>' first.");
    }

    Table* table = m_current_db->create_table(stmt.table_name, stmt.col_defs);
    if (!table) {
        set_error(String("Failed to create table: ") + stmt.table_name);
        return make_error_result(String("Failed to create table: ") + stmt.table_name);
    }
    return make_message_result(String("Table '") + stmt.table_name + "' created.");
}

ResultSet Executor::exec_drop_table(const SQLStatement& stmt) {
    if (!m_current_db) {
        set_error("No database selected");
        return make_error_result("No database selected.");
    }

    if (!m_current_db->drop_table(stmt.table_name)) {
        set_error(String("Table not found: ") + stmt.table_name);
        return make_error_result(String("Table not found: ") + stmt.table_name);
    }
    return make_message_result(String("Table '") + stmt.table_name + "' dropped.");
}

ResultSet Executor::exec_select(const SQLStatement& stmt) {
    if (!m_current_db) {
        set_error("No database selected");
        return make_error_result("No database selected.");
    }

    Table* table = m_current_db->get_table(stmt.table_name);
    if (!table) {
        set_error(String("Table not found: ") + stmt.table_name);
        return make_error_result(String("Table not found: ") + stmt.table_name);
    }

    ArrayList<String> cols;
    if (stmt.select_all) {
        const ArrayList<Column>& col_defs = table->columns();
        for (size_t i = 0; i < col_defs.size(); ++i) {
            cols.push_back(col_defs[i].name);
        }
    } else {
        cols = stmt.select_cols;
    }

    if (stmt.has_where) {
        Condition cond;
        cond.column_name = stmt.where_col;
        cond.op = stmt.where_op;
        cond.value = stmt.where_value;
        return table->select(cols, &cond);
    }
    return table->select(cols, nullptr);
}

ResultSet Executor::exec_insert(const SQLStatement& stmt) {
    if (!m_current_db) {
        set_error("No database selected");
        return make_error_result("No database selected.");
    }

    Table* table = m_current_db->get_table(stmt.table_name);
    if (!table) {
        set_error(String("Table not found: ") + stmt.table_name);
        return make_error_result(String("Table not found: ") + stmt.table_name);
    }

    const ArrayList<Column>& col_defs = table->columns();
    if (stmt.insert_values.size() != col_defs.size()) {
        set_error("Column count mismatch in INSERT");
        return make_error_result(
            String("Column count mismatch: expected ") +
            String(std::to_string(col_defs.size()).c_str()) +
            String(" but got ") +
            String(std::to_string(stmt.insert_values.size()).c_str()));
    }

    Json row_data = Json::object();
    for (size_t i = 0; i < col_defs.size(); ++i) {
        row_data.insert(col_defs[i].name, stmt.insert_values[i]);
    }

    if (!table->insert(row_data)) {
        set_error("Insert failed (possible primary key conflict)");
        return make_error_result("Insert failed: primary key conflict or I/O error.");
    }

    return make_message_result("1 row inserted.");
}

ResultSet Executor::exec_delete(const SQLStatement& stmt) {
    if (!m_current_db) {
        set_error("No database selected");
        return make_error_result("No database selected.");
    }

    Table* table = m_current_db->get_table(stmt.table_name);
    if (!table) {
        set_error(String("Table not found: ") + stmt.table_name);
        return make_error_result(String("Table not found: ") + stmt.table_name);
    }

    if (stmt.has_where) {
        Condition cond;
        cond.column_name = stmt.where_col;
        cond.op = stmt.where_op;
        cond.value = stmt.where_value;
        bool removed = table->remove(cond);
        if (!removed) {
            set_error("No matching row found");
            return make_error_result("No matching row to delete.");
        }
    } else {
        const ArrayList<Column>& col_defs = table->columns();
        if (col_defs.size() > 0) {
            Condition cond;
            cond.column_name = col_defs[0].name;
            cond.op = ConditionOp::GE;
            cond.value = Json(0);
            table->remove(cond);
        }
    }

    return make_message_result("Rows deleted.");
}

ResultSet Executor::exec_update(const SQLStatement& stmt) {
    if (!m_current_db) {
        set_error("No database selected");
        return make_error_result("No database selected.");
    }

    Table* table = m_current_db->get_table(stmt.table_name);
    if (!table) {
        set_error(String("Table not found: ") + stmt.table_name);
        return make_error_result(String("Table not found: ") + stmt.table_name);
    }

    Json new_vals = Json::object();
    new_vals.insert(stmt.set_col, stmt.set_value);

    if (stmt.has_where) {
        Condition cond;
        cond.column_name = stmt.where_col;
        cond.op = stmt.where_op;
        cond.value = stmt.where_value;
        bool updated = table->update(cond, new_vals);
        if (!updated) {
            set_error("No matching row found");
            return make_error_result("No matching row to update.");
        }
    } else {
        const ArrayList<Column>& tbl_cols = table->columns();
        if (tbl_cols.size() > 0) {
            Condition cond;
            cond.column_name = tbl_cols[0].name;
            cond.op = ConditionOp::GE;
            cond.value = Json(0);
            table->update(cond, new_vals);
        }
    }

    return make_message_result("Rows updated.");
}
