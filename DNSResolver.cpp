//
// Created by fffatigue on 26.12.18.
//

#include <fcntl.h>
#include <errno.h>
#include <string>
#include <sys/socket.h>
#include <fcntl.h>
#include <bits/select.h>
#include "DNSResolver.h"

DNSResolver::DNSResolver(unsigned char *host) : sockfd(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), drop(false) {
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
    dns = (struct DNS_HEADER *) buf;
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr("8.8.8.8"); //dns servers

    int con = connect(sockfd, (struct sockaddr *) &dest, sizeof(dest));

    dns->id = (unsigned short) htons(getpid());
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    dns->rd = 1; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;
    qname = (unsigned char *) &buf[sizeof(struct DNS_HEADER)];
    ChangetoDnsNameFormat(qname, host);
    struct QUESTION *qinfo = (struct QUESTION *) &buf[sizeof(struct DNS_HEADER) +
                                                      (strlen((const char *) qname) + 1)]; //fill it

    qinfo->qtype = htons(1); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(1);
}


bool DNSResolver::exchange_data(const fd_set &rdfds, const fd_set &wrfds) {
    if (FD_ISSET(sockfd, &wrfds) && !requestSended) send();
    if (FD_ISSET(sockfd, &rdfds) && !responseRecived) {
        reciveAndParse();
        return true;
    }
    return false;
}

long DNSResolver::getaddr() {
    return s_addr;
}

void DNSResolver::send() {
    if (::send(sockfd, (char *) buf,
               sizeof(struct DNS_HEADER) + (strlen((const char *) qname) + 1) + sizeof(struct QUESTION), 0) < 0) {
        if (errno != EWOULDBLOCK) {
            drop = true;
        }
        return;
    }
    requestSended = true;
}

void DNSResolver::reciveAndParse() {
    if (::recv(sockfd, (char *) buf, 65536, 0) <= 0) {
        if (errno != EWOULDBLOCK) {
            drop = true;
        }
        return;
    }
    responseRecived = true;

    struct RES_RECORD answers[20];
    unsigned char *reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char *) qname) + 1) +
                                 sizeof(struct QUESTION)];

    //Start reading answers
    int stop = 0;
    for (int i = 0; i < 2; i++) {
        answers[i].name = ReadName(reader, buf, &stop);
        reader = reader + stop;

        answers[i].resource = (struct R_DATA *) (reader);
        reader = reader + sizeof(struct R_DATA);

        if (ntohs(answers[i].resource->type) == 1) //if its an ipv4 address
        {
            answers[i].rdata = (unsigned char *) malloc(ntohs(answers[i].resource->data_len) + 1);

            for (int j = 0; j < ntohs(answers[i].resource->data_len); j++) {
                answers[i].rdata[j] = reader[j];
            }

            answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

            reader = reader + ntohs(answers[i].resource->data_len);
        } else {
            answers[i].rdata = ReadName(reader, buf, &stop);
            reader = reader + stop;
        }
    }
    if (ntohs(dns->ans_count) > 1)
        s_addr = *((long *) answers[1].rdata);
    else if (ntohs(dns->ans_count) > 0)
        s_addr = *((long *) answers[0].rdata);
    for (int i = 0; i < 2; i++) {
               free(answers[i].rdata);
               free(answers[i].name);
    }
}

DNSResolver::~DNSResolver() {
    close(sockfd);
}

void DNSResolver::fill_fd_set(fd_set &rdfds, fd_set &wrfds) {
    if (!requestSended) {
        FD_SET(sockfd, &wrfds);
    } else if (!responseRecived) {
        FD_SET(sockfd, &rdfds);
    }
}

int DNSResolver::getfd() {
    return sockfd;
}

bool DNSResolver::needToDrop() {
    return drop;
}

void DNSResolver::ChangetoDnsNameFormat(unsigned char *dns, unsigned char *host) {
    int lock = 0, i;
    char tmp[strlen(reinterpret_cast<const char *>(host)) + 1];
    strcpy(tmp, reinterpret_cast<const char *>(host));
    strcat(tmp, ".");
    for (i = 0; i < strlen(tmp); i++) {
        if (tmp[i] == '.') {
            *dns++ = i - lock;
            for (; lock < i; lock++) {
                *dns++ = tmp[lock];
            }
            lock++; //or lock=i+1;
        }
    }
    *dns++ = '\0';
}

unsigned char *DNSResolver::ReadName(unsigned char *reader, unsigned char *buffer, int *count) {
    unsigned char *name;
    unsigned int p = 0, jumped = 0, offset;
    int i, j;

    *count = 1;
    name = (unsigned char *) malloc(256);

    name[0] = '\0';

    //read the names in 3www6google3com format
    while (*reader != 0) {
        if (*reader >= 192) {
            offset = (*reader) * 256 + *(reader + 1) - 49152; //49152 = 11000000 00000000 ;)
            reader = buffer + offset - 1;
            jumped = 1; //we have jumped to another location so counting wont go up!
        } else {
            name[p++] = *reader;
        }

        reader = reader + 1;

        if (jumped == 0) {
            *count = *count + 1; //if we havent jumped to another location then we can count up
        }
    }

    name[p] = '\0'; //string complete
    if (jumped == 1) {
        *count = *count + 1; //number of steps we actually moved forward in the packet
    }

    //now convert 3www6google3com0 to www.google.com
    for (i = 0; i < (int) strlen((const char *) name); i++) {
        p = name[i];
        for (j = 0; j < (int) p; j++) {
            name[i] = name[i + 1];
            i = i + 1;
        }
        name[i] = '.';
    }
    if((int) strlen((const char *) name)>0) {
        name[i - 1] = '\0'; //remove the last dot
    }
    return name;
}



