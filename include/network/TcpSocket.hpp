#pragma once

#include <cstdint>
#include <cstddef>
#include "common/String.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    using SOCKET = int;
    static constexpr int INVALID_SOCKET = -1;
    static constexpr int SOCKET_ERROR   = -1;
#endif

class TcpSocket {
public:
    TcpSocket();
    ~TcpSocket();

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    bool create();
    bool bind(uint16_t port);
    bool listen(int backlog = 5);
    bool accept(TcpSocket& client_socket);
    bool connect(const char* host, uint16_t port);
    int  send_data(const void* data, size_t len);
    int  recv_data(void* buffer, size_t len);
    void close_socket();

    bool is_valid() const;
    SOCKET native_handle() const { return m_socket; }

    static bool initialize_network();
    static void shutdown_network();

    String last_error() const { return m_last_error; }

private:
    SOCKET m_socket;
    String m_last_error;
    bool   m_owned;

    void set_error(const char* msg);
    void release();
};
