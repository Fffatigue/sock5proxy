//
// Created by fffatigue on 26.12.18.
//

#ifndef FORWARDER_DNSRESOLVER_H
#define FORWARDER_DNSRESOLVER_H

#include<stdio.h> //printf
#include<string.h>    //strlen
#include<stdlib.h>    //malloc
#include<sys/socket.h>    //you know what this is for
#include<arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>    //getpid

class DNSResolver {
    struct DNS_HEADER {
        unsigned short id; // identification number

        unsigned char rd :1; // recursion desired
        unsigned char tc :1; // truncated message
        unsigned char aa :1; // authoritive answer
        unsigned char opcode :4; // purpose of message
        unsigned char qr :1; // query/response flag

        unsigned char rcode :4; // response code
        unsigned char cd :1; // checking disabled
        unsigned char ad :1; // authenticated data
        unsigned char z :1; // its z! reserved
        unsigned char ra :1; // recursion available

        unsigned short q_count; // number of question entries
        unsigned short ans_count; // number of answer entries
        unsigned short auth_count; // number of authority entries
        unsigned short add_count; // number of resource entries
    };

    struct QUESTION {
        unsigned short qtype;
        unsigned short qclass;
    };

#pragma pack(push, 1)
    struct R_DATA {
        unsigned short type;
        unsigned short _class;
        unsigned int ttl;
        unsigned short data_len;
    };
#pragma pack(pop)

//Pointers to resource record contents
    struct RES_RECORD {
        unsigned char* name;
        struct R_DATA *resource;
        unsigned char* rdata;
    };
    unsigned char buf[65536];
    int sockfd;
    struct DNS_HEADER *dns;
    unsigned char *qname;
    bool requestSended = false;
    bool responseRecived = false;
    bool drop;
    long s_addr;

public:
    bool needToDrop();

    explicit DNSResolver(unsigned char* host);

    bool exchange_data(const fd_set &rdfds, const fd_set &wrfds) ;

    long getaddr();

    void fill_fd_set(fd_set &rdfds, fd_set &wrfds);

    int getfd();

    ~DNSResolver();

private:

    void send();

    void reciveAndParse();

    void ChangetoDnsNameFormat(unsigned char *dns, unsigned char* host);

    unsigned char* ReadName(unsigned char *reader, unsigned char *buffer, int *count);

};

#endif //FORWARDER_DNSRESOLVER_H
