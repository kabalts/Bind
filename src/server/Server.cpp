#include "server/Server.hpp"
#include "parser/SQLParser.hpp"
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>

Server::Server(const String& data_dir)
    : m_data_dir(data_dir), m_listen_socket(), m_executor(data_dir),
      m_running(false), m_last_error() {}

Server::~Server() {
    stop();
}

uint32_t Server::read_uint32_be(const unsigned char* buf) {
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8)  |
           (static_cast<uint32_t>(buf[3]));
}

void Server::write_uint32_be(unsigned char* buf, uint32_t val) {
    buf[0] = static_cast<unsigned char>((val >> 24) & 0xFF);
    buf[1] = static_cast<unsigned char>((val >> 16) & 0xFF);
    buf[2] = static_cast<unsigned char>((val >> 8)  & 0xFF);
    buf[3] = static_cast<unsigned char>(val         & 0xFF);
}

bool Server::recv_message(TcpSocket& sock, String& out) {
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

bool Server::send_message(TcpSocket& sock, const String& data) {
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

bool Server::start(uint16_t port) {
    if (m_running) return true;

    if (!m_listen_socket.create()) {
        m_last_error = String("Failed to create socket: ") + m_listen_socket.last_error();
        return false;
    }
    if (!m_listen_socket.bind(port)) {
        m_last_error = String("Failed to bind port ") +
                       String(std::to_string(port).c_str()) +
                       String(": ") + m_listen_socket.last_error();
        return false;
    }
    if (!m_listen_socket.listen(5)) {
        m_last_error = String("Failed to listen: ") + m_listen_socket.last_error();
        return false;
    }

    m_running = true;
    std::cout << "Bind Server started on port " << port << std::endl;
    std::cout << "Data directory: " << m_data_dir << std::endl;

    while (m_running) {
        TcpSocket client_socket;
        if (!client_socket.create()) continue;

        std::cout << "Waiting for client connection..." << std::endl;
        if (!m_listen_socket.accept(client_socket)) {
            m_last_error = String("Accept failed: ") + m_listen_socket.last_error();
            continue;
        }

        std::cout << "Client connected." << std::endl;
        handle_client(client_socket);
        std::cout << "Client disconnected." << std::endl;
    }

    return true;
}

void Server::stop() {
    m_running = false;
    m_listen_socket.close_socket();
}

void Server::handle_client(TcpSocket& client_socket) {
    while (m_running) {
        String request_str;
        if (!recv_message(client_socket, request_str)) {
            break;
        }

        Json request = Json::parse(request_str);
        String response = process_request(request);

        if (!send_message(client_socket, response)) {
            break;
        }
    }
}

String Server::process_request(const Json& request) {
    String command;
    if (request.has_key("command")) {
        command = request["command"].as_string();
    }

    if (command == "sql") {
        String sql;
        if (request.has_key("sql")) {
            sql = request["sql"].as_string();
        }

        if (sql.empty()) {
            Json resp = Json::object();
            resp.insert("status", Json("error"));
            resp.insert("message", Json("Empty SQL statement"));
            return resp.serialize();
        }

        String db_name;
        if (request.has_key("db")) {
            db_name = request["db"].as_string();
        }

        if (!db_name.empty()) {
            SQLParser use_parser;
            SQLStatement use_stmt = use_parser.parse(String("use ") + db_name);
            if (use_stmt.error_msg.empty() && use_stmt.type == StmtType::USE) {
                m_executor.execute(use_stmt);
                String e = m_executor.error_message();
                if (!e.empty()) {
                    Json resp = Json::object();
                    resp.insert("status", Json("error"));
                    resp.insert("message", Json(String("Cannot switch database: ") + e));
                    return resp.serialize();
                }
            }
        }

        SQLParser parser;
        SQLStatement stmt = parser.parse(sql);

        if (!stmt.error_msg.empty()) {
            Json resp = Json::object();
            resp.insert("status", Json("error"));
            resp.insert("message", Json(String("Parse error: ") + stmt.error_msg));
            return resp.serialize();
        }

        if (stmt.type == StmtType::EXIT || stmt.type == StmtType::NONE) {
            Json resp = Json::object();
            resp.insert("status", Json("ok"));
            resp.insert("message", Json("bye"));
            resp.insert("result", Json::object());
            return resp.serialize();
        }

        ResultSet rs = m_executor.execute(stmt);
        String exec_error = m_executor.error_message();

        if (!exec_error.empty()) {
            Json resp = Json::object();
            resp.insert("status", Json("error"));
            resp.insert("message", Json(exec_error));
            return resp.serialize();
        }

        Json resp = Json::object();
        resp.insert("status", Json("ok"));
        resp.insert("message", Json("Query executed successfully"));

        Json result = result_set_to_json(rs, "");
        resp.insert("result", std::move(result));

        return resp.serialize();
    }

    Json resp = Json::object();
    resp.insert("status", Json("error"));
    resp.insert("message", Json(String("Unknown command: ") + command));
    return resp.serialize();
}

Json Server::result_set_to_json(const ResultSet& rs, const String&) const {
    Json result = Json::object();

    Json columns_arr = Json::array();
    const auto& col_names = rs.column_names();
    for (size_t i = 0; i < col_names.size(); ++i) {
        columns_arr.push_back(Json(col_names[i]));
    }
    result.insert("columns", std::move(columns_arr));

    Json types_arr = Json::array();
    const auto& col_types = rs.column_types();
    for (size_t i = 0; i < col_types.size(); ++i) {
        const char* type_str = "null";
        switch (col_types[i]) {
        case ColumnType::Int:    type_str = "int";    break;
        case ColumnType::Double: type_str = "double"; break;
        case ColumnType::String: type_str = "string"; break;
        case ColumnType::Bool:   type_str = "bool";   break;
        default: break;
        }
        types_arr.push_back(Json(type_str));
    }
    result.insert("column_types", std::move(types_arr));

    Json rows_arr = Json::array();
    for (size_t r = 0; r < rs.row_count(); ++r) {
        Json row_arr = Json::array();
        const auto& row = rs.get_row(r);
        for (size_t c = 0; c < row.size(); ++c) {
            row_arr.push_back(row[c]);
        }
        rows_arr.push_back(std::move(row_arr));
    }
    result.insert("rows", std::move(rows_arr));

    return result;
}
