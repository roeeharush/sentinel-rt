#include "common/network/Socket.h"
#include <unistd.h>
#include <sys/socket.h>
#include <csignal>
#include <cerrno>
#include <chrono>
#include <sys/time.h>

Socket::Socket(int fd) : fd_(fd) {}

Socket::~Socket() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

int Socket::fd() const {
    return fd_;
}

bool Socket::sendAll(const std::vector<std::uint8_t>& data) {
    std::size_t totalSent = 0;
    while (totalSent < data.size()) {
        ssize_t sent = send(fd_, data.data() + totalSent, data.size() - totalSent, 0);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        totalSent += static_cast<std::size_t>(sent);
    }
    return true;
}

std::optional<std::vector<std::uint8_t>> Socket::receive(std::size_t maxBytes) {
    std::vector<std::uint8_t> buffer(maxBytes);
    while (true) {
        ssize_t received = recv(fd_, buffer.data(), buffer.size(), 0);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return std::nullopt;
        }
        if (received == 0) {
            return std::nullopt;
        }
        buffer.resize(static_cast<std::size_t>(received));
        return buffer;
    }
}

void ignoreSigPipe() {
    signal(SIGPIPE, SIG_IGN);
}

void Socket::enableAddressReuse() {
    int enable = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
}

void Socket::setReceiveTimeout(std::chrono::milliseconds timeout) {
    timeval tv{};
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}