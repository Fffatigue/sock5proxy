#include <sys/select.h>
#include <sys/socket.h>
#include <stdexcept>
#include <list>
#include <iostream>
#include <cerrno>
#include "utils.hpp"
#include "connection.hpp"
#include <algorithm>
#include <signal.h>
#include <fcntl.h>
#include "server.hpp"

Server::Server(int listen_port) :
        listen_sockfd(socket(AF_INET, SOCK_STREAM, 0)),
        maxfd(0) {

    signal(SIGPIPE, SIG_IGN);
    if (!add_fd(listen_sockfd)) {
        throw std::runtime_error(std::string("Socket number out of range for select"));
    }
    int opt = 1;
    setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in listenaddr = Utils::get_sockaddr(NULL, listen_port);


    if (bind(listen_sockfd, reinterpret_cast<struct sockaddr *>(&listenaddr), sizeof(listenaddr))) {
        throw std::runtime_error(std::string("bind: ") + strerror(errno));
    }

    signal(SIGPIPE, SIG_IGN);

    if (listen(listen_sockfd, SOMAXCONN)) {
        throw std::runtime_error(std::string("listen: ") + strerror(errno));
    }
    if (fcntl(listen_sockfd, F_SETFL, fcntl(listen_sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        throw std::runtime_error("Can't make server socket nonblock!");
    }
}

void Server::run() {
    while (true) {
        prepare_sets();
        int selected = select(maxfd + 1, &rdfds, &wrfds, NULL, NULL);
        if (selected < 0) {
            throw std::runtime_error(std::string("select: ") + strerror(errno));
        } else if (selected > 0) {
            process_pending_clients();
            process_waiting_for_handshake();
            process_waiting_for_resolve();
            process_connections();
        }
    }
}

bool Server::add_fd(int sockfd) {
    maxfd = std::max(maxfd, sockfd);
    return sockfd < FD_SETSIZE;
}

void Server::prepare_sets() {
    FD_ZERO(&rdfds);
    FD_ZERO(&wrfds);
    FD_SET(listen_sockfd, &rdfds);

    for (auto i = connections.begin(); i != connections.end();) {
        if ((*i)->is_active()) {
            (*i)->fill_fd_set(rdfds, wrfds);
        } else {
            delete(*i);
            connections.erase(i++);
            continue;
        }
        ++i;
    }
    for (auto i : waitingForHandshake){
        i.second->fill_fd_set(rdfds,wrfds);
    }
    for(auto i : waitingForResolve){
        i.second->fill_fd_set(rdfds,wrfds);
        add_fd(i.second->getfd());
    }

}

void Server::process_connections() {
    for (auto i = connections.begin(); i != connections.end(); ++i) {
        (*i)->exchange_data(rdfds, wrfds);
    }
}

void Server::process_pending_clients() {
    if (FD_ISSET(listen_sockfd, &rdfds)) {
        sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_connection = accept(listen_sockfd, reinterpret_cast<sockaddr *>(&client_addr), &addrlen);
        if (!add_fd(client_connection)) {
            throw std::runtime_error(std::string("Socket number out of range for select"));
        }
        waitingForHandshake.push_back(std::pair<int, SOCK5ConnectionMaker*>(client_connection,new SOCK5ConnectionMaker(client_connection)));
    }
}

void Server::process_waiting_for_handshake() {
    for (auto i = waitingForHandshake.begin();i!=waitingForHandshake.end();) {
        int code = i->second->exchange_data(rdfds,wrfds);
        if(code == 1){
            if(i->second->isDomain()){
                std::pair<int,int> fd_port = std::pair<int,int>(i->first,i->second->getPort());
                waitingForResolve.push_back(std::pair<std::pair<int, int>, DNSResolver*>(fd_port,new DNSResolver((unsigned char*)i->second->getHost().c_str())));
            } else{
                int forwarding_socket = socket(AF_INET, SOCK_STREAM, 0);
                sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_addr.s_addr = inet_addr(i->second->getHost().c_str());
                addr.sin_port = htons(i->second->getPort());
                connections.emplace_back(new Connection(i->first,forwarding_socket,&addr));
                if (!add_fd(forwarding_socket)) {
                    throw std::runtime_error(std::string("Socket number out of range for select"));
                }
            }
            delete(i->second);
            waitingForHandshake.erase(i++);
        }else{
            i++;
        }
    }

}

void Server::process_waiting_for_resolve() {
    for (auto i = waitingForResolve.begin();i!=waitingForResolve.end();) {
        if (i->second->exchange_data(rdfds, wrfds)) {
            int forwarding_socket = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_addr.s_addr = i->second->getaddr();
            addr.sin_port = htons(i->first.second);
            addr.sin_family = AF_INET;
            connections.push_back(new Connection(i->first.first, forwarding_socket, &addr));
            if (!add_fd(forwarding_socket)) {
                throw std::runtime_error(std::string("Socket number out of range for select"));
            }
            delete(i->second);
            waitingForResolve.erase(i++);
        }else{
            add_fd(i->second->getfd());
            i++;
        }
    }

}
