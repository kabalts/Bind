#include "network/TcpSocket.hpp"
#include <cstring>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

TcpSocket::TcpSocket() : m_socket(INVALID_SOCKET), m_last_error(), m_owned(true) {}

TcpSocket::~TcpSocket() {
    if (m_owned) close_socket();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : m_socket(other.m_socket), m_last_error(std::move(other.m_last_error)),
      m_owned(other.m_owned) {
    other.m_socket = INVALID_SOCKET;
    other.m_owned = false;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        if (m_owned) close_socket();
        m_socket = other.m_socket;
        m_last_error = std::move(other.m_last_error);
        m_owned = other.m_owned;
        other.m_socket = INVALID_SOCKET;
        other.m_owned = false;
    }
    return *this;
}

bool TcpSocket::initialize_network() {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    return result == 0;
}

void TcpSocket::shutdown_network() {
    WSACleanup();
}

void TcpSocket::set_error(const char* msg) {
    char buf[256];
    int err = WSAGetLastError();
    std::snprintf(buf, sizeof(buf), "%s (errno=%d)", msg, err);
    m_last_error = buf;
}

void TcpSocket::release() {
    m_socket = INVALID_SOCKET;
    m_owned = false;
}

bool TcpSocket::create() {
    if (m_owned) close_socket();
    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        set_error("socket() failed");
        return false;
    }
    m_owned = true;

    int opt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    return true;
}

bool TcpSocket::bind(uint16_t port) {
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        set_error("bind() failed");
        return false;
    }
    return true;
}

bool TcpSocket::listen(int backlog) {
    if (::listen(m_socket, backlog) == SOCKET_ERROR) {
        set_error("listen() failed");
        return false;
    }
    return true;
}

bool TcpSocket::accept(TcpSocket& client_socket) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    std::memset(&client_addr, 0, sizeof(client_addr));

    SOCKET client_fd = ::accept(m_socket, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd == INVALID_SOCKET) {
        set_error("accept() failed");
        return false;
    }

    client_socket.close_socket();
    client_socket.m_socket = client_fd;
    client_socket.m_owned = true;
    return true;
}

bool TcpSocket::connect(const char* host, uint16_t port) {
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(host);
        if (!he) {
            set_error("Failed to resolve hostname");
            return false;
        }
        std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        set_error("connect() failed");
        return false;
    }
    return true;
}

int TcpSocket::send_data(const void* data, size_t len) {
    int sent = ::send(m_socket, (const char*)data, static_cast<int>(len), 0);
    if (sent == SOCKET_ERROR) {
        set_error("send() failed");
    }
    return sent;
}

int TcpSocket::recv_data(void* buffer, size_t len) {
    int received = ::recv(m_socket, (char*)buffer, static_cast<int>(len), 0);
    if (received == SOCKET_ERROR) {
        set_error("recv() failed");
    }
    return received;
}

void TcpSocket::close_socket() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool TcpSocket::is_valid() const {
    return m_socket != INVALID_SOCKET;
}
