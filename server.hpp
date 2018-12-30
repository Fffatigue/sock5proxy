#ifndef PFWD_SERVER_HPP
#define PFWD_SERVER_HPP

#include <sys/select.h>
#include <sys/socket.h>
#include <stdexcept>
#include <list>
#include <iostream>
#include <cerrno>
#include <algorithm>
#include "utils.hpp"
#include "connection.hpp"
#include "DNSResolver.h"
#include "SOCK5ConnectionMaker.h"

class Server {
    fd_set rdfds, wrfds;
    std::list<std::pair<std::pair<int, int>, DNSResolver*> > waitingForResolve;
    std::list<std::pair<int, SOCK5ConnectionMaker*>> waitingForHandshake;
    std::list<Connection*> connections;
    int listen_sockfd;
    int maxfd;
public:
    Server(int listen_port);

    void run();

private:
    bool add_fd(int sockfd);

    void prepare_sets();

    void process_connections();

    void process_pending_clients();

    void process_waiting_for_resolve();

    void process_waiting_for_handshake();
};

#endif
