/* 
 * simple ICMP tunnel
 * vim:set sw=4 sta et:
 *
 * Copyright (C) 2009  Tomas Janousek <tomi@nomi.cz>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include "ping.h"

int init_udp_socket(unsigned short port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        perror("socket"), abort();

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (const struct sockaddr *) &addr,
                sizeof(struct sockaddr_in)) == -1)
        perror("bind"), abort();

    return sock;
}

void resolv(const char *host, struct sockaddr_in* addr)
{
    struct hostent *he;

    if ((he = gethostbyname(host)) == NULL)
        fprintf(stderr, "gethostbyname: %s\n", hstrerror(h_errno)), abort();

    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    memcpy(&addr->sin_addr, he->h_addr, he->h_length);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        fprintf(stderr, "./client port host key\n"), abort();

    int port = atoi(argv[1]);
    struct sockaddr_in host; resolv(argv[2], &host);
    int key = atoi(argv[3]);

    int sock = init_ping_socket();
    int udpsock = init_udp_socket(port);

    struct sockaddr_in lastaddr;
    int lastaddr_valid = 0;

    fd_set fds;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        FD_SET(udpsock, &fds);

        if (select(udpsock + 1, &fds, NULL, NULL, NULL) == -1)
            perror("select"), abort();

        if (FD_ISSET(sock, &fds)) {
            struct sockaddr_in addr;
            u_int16_t id, seq;
            char *data = NULL;
            ssize_t len;

            if (!recv_ping(sock, &addr, &id, &seq, &data, &len))
                goto ignore_ping;

            if ((id ^ seq) != key)
                goto ignore_ping;

            if (!lastaddr_valid)
                goto ignore_ping;

            if (sendto(udpsock, data, len, 0, (struct sockaddr *) &lastaddr,
                    sizeof(struct sockaddr_in)) == -1)
                perror("sendto"), abort();

ignore_ping:
            free(data);
        }

        if (FD_ISSET(udpsock, &fds)) {
            char buffer[4096];
            socklen_t addrlen = sizeof(struct sockaddr_in);
            ssize_t len;

            if ((len = recvfrom(udpsock, buffer, 4096, 0,
                    (struct sockaddr *) &lastaddr, &addrlen)) == -1) {
                if (errno == ECONNREFUSED)
                    goto ignore_udp_fail;
                perror("recvfrom"), abort();
            }

            unsigned short x = rand();
            send_ping(sock, &host, x, x ^ key, buffer, len);

            lastaddr_valid = 1;
        }
ignore_udp_fail: continue;
    }

    return 0;
}
