//
// Created by fffatigue on 26.12.18.
//

#ifndef FORWARDER_SOCK5CONNECTIONMAKER_H
#define FORWARDER_SOCK5CONNECTIONMAKER_H


#include <sys/types.h>
#include <string>

class SOCK5ConnectionMaker {
private:
    bool drop;
    const static int MAX_BUF_SIZE = 1024;
    int client_sock_;
    unsigned char inputbuf[MAX_BUF_SIZE];
    unsigned char outputbuf[MAX_BUF_SIZE];
    int inputbuf_size;
    int outputbuf_size;
    bool firstStepCompleted;
    bool domain;
    int port;
    std::string addr;
    int server_port;
public:
    bool needToDrop();

    SOCK5ConnectionMaker(int client_sock);

    void fill_fd_set(fd_set &rdfds, fd_set &wrfds);

    std::string getHost();

    int getPort();

    bool isDomain();
    int exchange_data(const fd_set &rdfds, const fd_set &wrfds);

};


#endif //FORWARDER_SOCK5CONNECTIONMAKER_H
