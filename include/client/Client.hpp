#pragma once

#include "common/String.hpp"
#include "common/Json.hpp"
#include "common/ArrayList.hpp"
#include "network/TcpSocket.hpp"

class Client {
public:
    Client();
    ~Client();

    bool connect(const char* host, uint16_t port, const String& default_db = "");
    void disconnect();
    bool is_connected() const;

    void run();

    const String& last_error() const { return m_last_error; }

private:
    TcpSocket m_socket;
    String    m_default_db;
    String    m_pending_db;
    String    m_last_error;
    bool      m_connected;

    bool send_request(const String& sql);
    Json receive_response();

    bool recv_message(TcpSocket& sock, String& out);
    bool send_message(TcpSocket& sock, const String& data);

    void print_result(const Json& response);
    void print_table(const Json& result);
    void print_error(const String& message);
    void print_separator(const ArrayList<size_t>& widths);
    String pad_right(const String& s, size_t width);
    void print_usage();

    static uint32_t read_uint32_be(const unsigned char* buf);
    static void     write_uint32_be(unsigned char* buf, uint32_t val);
};
