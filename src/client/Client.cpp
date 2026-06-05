#include "client/Client.hpp"
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>

#include <io.h>
#define isatty _isatty
#define STDIN_FILENO 0

Client::Client() : m_socket(), m_default_db(), m_last_error(), m_connected(false) {}

Client::~Client() {
    disconnect();
}

uint32_t Client::read_uint32_be(const unsigned char* buf) {
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8)  |
           (static_cast<uint32_t>(buf[3]));
}

void Client::write_uint32_be(unsigned char* buf, uint32_t val) {
    buf[0] = static_cast<unsigned char>((val >> 24) & 0xFF);
    buf[1] = static_cast<unsigned char>((val >> 16) & 0xFF);
    buf[2] = static_cast<unsigned char>((val >> 8)  & 0xFF);
    buf[3] = static_cast<unsigned char>(val         & 0xFF);
}

bool Client::recv_message(TcpSocket& sock, String& out) {
    unsigned char header[4];
    int total = 0;
    while (total < 4) {
        int n = sock.recv_data(header + total, 4 - total);
        if (n <= 0) return false;
        total += n;
    }

    uint32_t msg_len = read_uint32_be(header);
    if (msg_len > 16 * 1024 * 1024) return false;

    char* buffer = new char[msg_len + 1];
    total = 0;
    while (total < static_cast<int>(msg_len)) {
        int n = sock.recv_data(buffer + total, msg_len - total);
        if (n <= 0) { delete[] buffer; return false; }
        total += n;
    }
    buffer[msg_len] = '\0';
    out = String(buffer, msg_len);
    delete[] buffer;
    return true;
}

bool Client::send_message(TcpSocket& sock, const String& data) {
    unsigned char header[4];
    write_uint32_be(header, static_cast<uint32_t>(data.length()));

    int sent = sock.send_data(header, 4);
    if (sent != 4) return false;

    size_t offset = 0;
    const char* ptr = data.c_str();
    while (offset < data.length()) {
        int n = sock.send_data(ptr + offset, data.length() - offset);
        if (n <= 0) return false;
        offset += n;
    }
    return true;
}

bool Client::connect(const char* host, uint16_t port, const String& default_db) {
    disconnect();

    if (!m_socket.create()) {
        m_last_error = String("Failed to create socket: ") + m_socket.last_error();
        return false;
    }

    if (!m_socket.connect(host, port)) {
        m_last_error = String("Failed to connect to ") + host +
                       String(":") + String(std::to_string(port).c_str()) +
                       String(" - ") + m_socket.last_error();
        return false;
    }

    m_default_db = default_db;
    m_connected = true;
    return true;
}

void Client::disconnect() {
    m_connected = false;
    m_socket.close_socket();
}

bool Client::is_connected() const {
    return m_connected && m_socket.is_valid();
}

bool Client::send_request(const String& sql) {
    Json request = Json::object();
    request.insert("command", Json("sql"));
    request.insert("sql", Json(sql));
    if (!m_default_db.empty()) {
        request.insert("db", Json(m_default_db));
    }
    return send_message(m_socket, request.serialize());
}

Json Client::receive_response() {
    String data;
    if (!recv_message(m_socket, data)) {
        return Json();
    }
    return Json::parse(data);
}

void Client::print_usage() {
    std::cout << std::endl;
    std::cout << "Bind Client - Supported Commands:" << std::endl;
    std::cout << "  SQL statements:" << std::endl;
    std::cout << "    CREATE DATABASE <name>" << std::endl;
    std::cout << "    DROP DATABASE <name>" << std::endl;
    std::cout << "    USE <dbname>" << std::endl;
    std::cout << "    CREATE TABLE <name> (<col> <type>, ...)" << std::endl;
    std::cout << "    DROP TABLE <name>" << std::endl;
    std::cout << "    SELECT <cols>|* FROM <name> [WHERE <cond>]" << std::endl;
    std::cout << "    INSERT INTO <name> VALUES (...)" << std::endl;
    std::cout << "    DELETE FROM <name> [WHERE <cond>]" << std::endl;
    std::cout << "    UPDATE <name> SET <col> = <val> [WHERE <cond>]" << std::endl;
    std::cout << "  Special commands:" << std::endl;
    std::cout << "    help  - Show this help" << std::endl;
    std::cout << "    exit  - Exit client" << std::endl;
    std::cout << std::endl;
}

void Client::run() {
    if (!is_connected()) {
        std::cerr << "Not connected to server." << std::endl;
        return;
    }

    bool interactive = (isatty(STDIN_FILENO) != 0);

    if (interactive) {
        std::cout << "Bind Client" << std::endl;
        std::cout << "Type 'help' for usage, 'exit' to quit." << std::endl;
        std::cout << std::endl;
    }

    std::string line;
    while (true) {
        if (interactive) {
            if (m_default_db.empty()) {
                std::cout << "Bind> ";
            } else {
                std::cout << "Bind/" << m_default_db.c_str() << "> ";
            }
            std::cout.flush();
        }

        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) continue;

        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }

        String sql(line.c_str(), line.length());

        String lower = sql;
        for (size_t i = 0; i < lower.length(); ++i) {
            if (lower[i] >= 'A' && lower[i] <= 'Z') {
                lower[i] = static_cast<char>(lower[i] + ('a' - 'A'));
            }
        }

        if (lower == "exit" || lower == "quit") {
            std::cout << "Bye." << std::endl;
            break;
        }

        if (lower == "help") {
            print_usage();
            continue;
        }

        if (lower.find("use ") == 0) {
            size_t space = lower.find(' ');
            String db_name = sql.substr(space + 1);
            for (size_t i = 0; i < db_name.length(); ++i) {
                if (db_name[i] == ' ' || db_name[i] == ';') {
                    db_name = db_name.substr(0, i);
                    break;
                }
            }
            m_pending_db = db_name;
        }

        bool is_drop_db  = (lower.find("drop database ") == 0);
        bool is_drop_tbl = (lower.find("drop table ") == 0);
        if (interactive && (is_drop_db || is_drop_tbl)) {
            String target = is_drop_db ? "database" : "table";
            String obj_name;
             for (size_t j = sql.length(); j > 0; --j) {
                 if (sql[j - 1] == ' ') {
                     obj_name = sql.substr(j);
                     break;
                 }
             }
             for (size_t i = 0; i < obj_name.length(); ++i) {
                 if (obj_name[i] == ';') {
                     obj_name = obj_name.substr(0, i);
                     break;
                 }
             }
             if (obj_name.empty()) obj_name = "?";
            std::cout << "Drop " << target.c_str() << " '"
                      << obj_name.c_str() << "'? (y/N) ";
            std::cout.flush();
            std::string confirm;
            if (!std::getline(std::cin, confirm)) break;
            if (confirm != "y" && confirm != "Y" && confirm != "yes" && confirm != "Yes") {
                std::cout << "Cancelled." << std::endl;
                continue;
            }
        }

        if (!send_request(sql)) {
            std::cerr << "Error sending request: " << m_last_error.c_str() << std::endl;
            m_connected = false;
            break;
        }

        Json response = receive_response();

        if (!m_pending_db.empty()) {
            String status;
            if (response.has_key("status")) {
                status = response["status"].as_string();
            }
            if (status == "ok") {
                m_default_db = m_pending_db;
            }
            m_pending_db = String();
        }

        if (is_drop_db) {
            String status;
            if (response.has_key("status")) {
                status = response["status"].as_string();
            }
            if (status == "ok") {
                String dropped_db;
                for (size_t j = sql.length(); j > 0; --j) {
                    if (sql[j - 1] == ' ') {
                        dropped_db = sql.substr(j);
                        break;
                    }
                }
                for (size_t i = 0; i < dropped_db.length(); ++i) {
                    if (dropped_db[i] == ';') {
                        dropped_db = dropped_db.substr(0, i);
                        break;
                    }
                }
                if (dropped_db == m_default_db) {
                    m_default_db = String();
                }
            }
        }

        print_result(response);
    }
}

void Client::print_result(const Json& response) {
    if (response.is_null()) {
        std::cerr << "Error: Failed to receive response from server." << std::endl;
        return;
    }

    String status;
    if (response.has_key("status")) {
        status = response["status"].as_string();
    }

    if (status == "error") {
        String msg;
        if (response.has_key("message")) {
            msg = response["message"].as_string();
        }
        print_error(msg);
        return;
    }

    if (response.has_key("message")) {
        String msg = response["message"].as_string();
        if (!msg.empty() && msg != "Query executed successfully") {
            std::cout << msg.c_str() << std::endl;
        }
    }

    if (response.has_key("result")) {
        const Json& result = response["result"];
        if (result.is_object() && result.has_key("columns") &&
            result["columns"].is_array() && result["columns"].size() > 0) {
            print_table(result);
        }
    }
}

String Client::pad_right(const String& s, size_t width) {
    String result = s;
    while (result.length() < width) {
        result += " ";
    }
    return result;
}

void Client::print_separator(const ArrayList<size_t>& widths) {
    std::cout << "+";
    for (size_t i = 0; i < widths.size(); ++i) {
        for (size_t j = 0; j < widths[i] + 2; ++j) {
            std::cout << "-";
        }
        std::cout << "+";
    }
    std::cout << std::endl;
}

void Client::print_table(const Json& result) {
    const Json& columns = result["columns"];
    const Json& rows = result["rows"];

    size_t col_count = columns.size();
    ArrayList<size_t> widths;

    for (size_t i = 0; i < col_count; ++i) {
        widths.push_back(columns[i].as_string().length());
    }

    for (size_t r = 0; r < rows.size(); ++r) {
        const Json& row = rows[r];
        for (size_t c = 0; c < col_count && c < row.size(); ++c) {
            String cell;
            if (row[c].is_null()) {
                cell = "NULL";
            } else if (row[c].is_string()) {
                cell = row[c].as_string();
            } else if (row[c].is_number()) {
                double d = row[c].as_number();
                char buf[64];
                if (d == static_cast<long long>(d)) {
                    std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(d));
                } else {
                    std::snprintf(buf, sizeof(buf), "%.6g", d);
                }
                cell = String(buf);
            } else if (row[c].is_bool()) {
                cell = row[c].as_bool() ? "true" : "false";
            } else {
                cell = row[c].serialize();
            }
            if (cell.length() > widths[c]) {
                widths[c] = cell.length();
            }
        }
    }

    print_separator(widths);

    std::cout << "|";
    for (size_t i = 0; i < col_count; ++i) {
        std::cout << " " << pad_right(columns[i].as_string(), widths[i]).c_str() << " |";
    }
    std::cout << std::endl;

    print_separator(widths);

    for (size_t r = 0; r < rows.size(); ++r) {
        const Json& row = rows[r];
        std::cout << "|";
        for (size_t c = 0; c < col_count && c < row.size(); ++c) {
            String cell;
            if (row[c].is_null()) {
                cell = "NULL";
            } else if (row[c].is_string()) {
                cell = row[c].as_string();
            } else if (row[c].is_number()) {
                double d = row[c].as_number();
                char buf[64];
                if (d == static_cast<long long>(d)) {
                    std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(d));
                } else {
                    std::snprintf(buf, sizeof(buf), "%.6g", d);
                }
                cell = String(buf);
            } else if (row[c].is_bool()) {
                cell = row[c].as_bool() ? "true" : "false";
            } else {
                cell = row[c].serialize();
            }
            std::cout << " " << pad_right(cell, widths[c]).c_str() << " |";
        }
        std::cout << std::endl;
    }

    print_separator(widths);

    if (rows.size() == 1) {
        std::cout << "1 row in set" << std::endl << std::endl;
    } else {
        std::cout << rows.size() << " rows in set" << std::endl << std::endl;
    }
}

void Client::print_error(const String& message) {
    std::cerr << "ERROR: " << message.c_str() << std::endl;
}
