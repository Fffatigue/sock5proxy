#include <cstdlib>
#include <vector>
#include <string>
#include <netinet/in.h>
#include <cstdio>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include "connection.hpp"

Connection::Connection(int client_socket, int forwarding_socket, sockaddr_in *serveraddr) :
        connected(false),
        forwarding_socket_(forwarding_socket),
        serveraddr_(serveraddr),
        client_socket_(client_socket),
        kMaxTimeout(128),
        kBufSize(4096),
        data_c_f_(0),
        data_f_c_(0),
        active_(true) {
    fcntl(forwarding_socket_, F_SETFL, fcntl(forwarding_socket_, F_GETFL, 0) | O_NONBLOCK);
    buf_cf_ = std::vector<char>(kBufSize);
    buf_fc_ = std::vector<char>(kBufSize);
    if (forwarding_socket_ == -1) {
        throw std::runtime_error(std::string("socket: ") + strerror(errno));
    }
    connect();
}

Connection::~Connection() {
    if (connected) {
        disconnect();
    }
}

void Connection::connect() {
    int ret;
    for (int numsec = 1; numsec <= kMaxTimeout; numsec *= 2) {
        ret = ::connect(forwarding_socket_, reinterpret_cast<sockaddr *> (serveraddr_), sizeof(*serveraddr_));
        connected = true;
        return;
    }
    throw std::runtime_error(std::string("connect: ") + strerror(errno));
}

void Connection::disconnect() {
    close(client_socket_);
    close(forwarding_socket_);
    connected = false;
}


void Connection::fill_fd_set(fd_set &rdfds, fd_set &wrfds) {
    if (data_c_f_ == 0) {
        FD_SET(client_socket_, &rdfds);
    }
    if (data_f_c_ == 0) {
        FD_SET(forwarding_socket_, &rdfds);
    }
    if (data_c_f_ > 0) {
        FD_SET(forwarding_socket_, &wrfds);
    }
    if (data_f_c_ > 0) {
        FD_SET(client_socket_, &wrfds);
    }
}

void Connection::exchange_data(const fd_set &rdfds, const fd_set &wrfds) {
    if (data_c_f_ == 0 && FD_ISSET(client_socket_, &rdfds)) {
        data_c_f_ = ::recv(client_socket_, &buf_cf_[0], kBufSize, 0);
        if (data_c_f_ == 0 || (data_c_f_ == -1 && errno != EWOULDBLOCK)) {
            active_ = false;
        }
        if (data_c_f_ == -1 && errno == EWOULDBLOCK) {
            data_c_f_ = 0;
        }
    }
    if (data_f_c_ == 0 && FD_ISSET(forwarding_socket_, &rdfds)) {
        data_f_c_ = ::recv(forwarding_socket_, &buf_fc_[0], kBufSize, 0);
        if (data_f_c_ == 0 || (data_f_c_ == -1 && errno != EWOULDBLOCK)) {
            active_ = false;
        }
        if (data_f_c_ == -1 && errno == EWOULDBLOCK) {
            data_f_c_ = 0;
        }
    }
    if (data_c_f_ > 0 && FD_ISSET(forwarding_socket_, &wrfds)) {
        ssize_t res = send(forwarding_socket_, &buf_cf_[0], data_c_f_, 0);
        if (res == -1) {
            if (errno != EWOULDBLOCK) {
                active_ = false;
            }
        } else data_c_f_ = 0;
    }
    if (data_f_c_ > 0 && FD_ISSET(client_socket_, &wrfds)) {
        ssize_t res = send(client_socket_, &buf_fc_[0], data_f_c_, 0);
        if (res == -1) {
            if (errno != EWOULDBLOCK) {
                active_ = false;
            }
        } else data_f_c_ = 0;
    }

}

bool Connection::is_active() const {
    return active_;
}
