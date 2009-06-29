/* 
 * simple ICMP tunnel
 * vim:set sw=4 sta et:
 *
 * Copyright (C) 2009  Tomas Janousek <tomi@nomi.cz>
 */
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char *argv[])
{
    if (argc != 4)
        fprintf(stderr, "./client port host key\n"), abort();

    int port = atoi(argv[1]);
    struct sockaddr_in host; resolv(argv[2], &host);
    long key = atol(argv[3]);

    int sock = init_ping_socket();
    int udpsock = init_udp_socket(port);

    struct sockaddr_in lastaddr;
    int lastaddr_valid = 0;

    int nfds;
    fd_set fds;
    struct timeval tv;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        FD_SET(udpsock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        if ((nfds = select(udpsock + 1, &fds, NULL, NULL, &tv)) == -1)
            perror("select"), abort();

        if (FD_ISSET(sock, &fds)) {
            struct sockaddr_in addr;
            u_int8_t type, code;
            u_int16_t id, seq;
            char *data = NULL;
            ssize_t len;

            if (!recv_ping(sock, &addr, &type, &code, &id, &seq, &data, &len))
                goto ignore_ping;

            if (type != ICMP_ECHOREPLY || code != 0 || seq != 1)
                goto ignore_ping;

            if (len <= sizeof(long) || ntohl(* (long *) data) != key)
                goto ignore_ping;

            if (!lastaddr_valid)
                goto ignore_ping;

            if (sendto(udpsock, data + sizeof(long), len - sizeof(long), 0,
                    (struct sockaddr *) &lastaddr, sizeof(struct sockaddr_in)) == -1)
                perror("sendto"), abort();

ignore_ping:
            free(data);
        }

        if (FD_ISSET(udpsock, &fds)) {
            char buffer[4096 + sizeof(long)];
            socklen_t addrlen = sizeof(struct sockaddr_in);
            ssize_t len;

            * (long *) buffer = htonl(key);
            if ((len = recvfrom(udpsock, buffer + sizeof(long), 4096, 0,
                    (struct sockaddr *) &lastaddr, &addrlen)) == -1) {
                if (errno == ECONNREFUSED)
                    goto ignore_udp_fail;
                perror("recvfrom"), abort();
            }

            u_int16_t x = rand();
            send_ping(sock, &host, ICMP_ECHO, 0,
                    x, 0, buffer, len + sizeof(long));

            lastaddr_valid = 1;
        }
ignore_udp_fail:

        if (nfds == 0) {
            long xkey = htonl(key);

            u_int16_t x = rand();
            send_ping(sock, &host, ICMP_ECHO, 0,
                    x, 0, (char *) &xkey, sizeof(long));
        }
    }

    return 0;
}
