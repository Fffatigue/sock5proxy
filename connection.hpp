#ifndef PFWD_CONNECTION_HPP
#define PFWD_CONNECTION_HPP

#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

class Connection {
    const int kMaxTimeout;
    const size_t kBufSize;
    int client_socket_, forwarding_socket_;
    ssize_t data_c_f_, data_f_c_;
    sockaddr_in* serveraddr_;
    std::vector<char> buf_cf_;
    std::vector<char> buf_fc_;
    bool connected;
    bool active_;
public:
    Connection(int client_socket, int forwarding_socket, sockaddr_in* serveraddr);
    ~Connection();
    void connect();
    void disconnect();
    void fill_fd_set(fd_set& rdfds, fd_set& wrfds);
    void exchange_data(const fd_set& rdfds, const fd_set& wrfds);
    bool is_active() const;
};

#endif
