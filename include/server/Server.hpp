#pragma once

#include "common/String.hpp"
#include "common/Json.hpp"
#include "network/TcpSocket.hpp"
#include "executor/Executor.hpp"

class Server {
public:
    explicit Server(const String& data_dir = "data");
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    bool start(uint16_t port = 8888);
    void stop();
    bool is_running() const { return m_running; }

    const String& last_error() const { return m_last_error; }

private:
    String    m_data_dir;
    TcpSocket m_listen_socket;
    Executor  m_executor;
    bool      m_running;
    String    m_last_error;

    void handle_client(TcpSocket& client_socket);
    String process_request(const Json& request);
    Json    result_set_to_json(const ResultSet& rs, const String& message) const;

    bool recv_message(TcpSocket& sock, String& out);
    bool send_message(TcpSocket& sock, const String& data);

    static uint32_t read_uint32_be(const unsigned char* buf);
    static void     write_uint32_be(unsigned char* buf, uint32_t val);
};
