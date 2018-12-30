#ifndef PFWD_UTILS_HPP
#define PFWD_UTILS_HPP


#include <sys/socket.h>
#include <memory>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <stdlib.h>
#include <netdb.h>
#include <cerrno>
#include <cstring>

class Utils {
public:
    static int parse_port(char* s_port);
    static struct sockaddr_in get_sockaddr(char* name, int port);
private:
    static void prepare_hints(struct addrinfo* hints);
};

#endif