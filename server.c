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
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(sock, (const struct sockaddr *) &addr,
                sizeof(struct sockaddr_in)) == -1)
        perror("connect"), abort();

    return sock;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        fprintf(stderr, "./server port addr key\n"), abort();

    int port = atoi(argv[1]);
    struct sockaddr_in host; resolv(argv[2], &host);
    long key = atol(argv[3]);

    int sock = init_ping_socket();
    bind_ping_socket(sock, &host);
    int udpsock = init_udp_socket(port);

    struct sockaddr_in lastaddr;
#define POOLSZ 16
    u_int16_t id_pool[POOLSZ];
    int id_pool_r = 0, id_pool_w = 0;

    fd_set fds;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        FD_SET(udpsock, &fds);

        if (select(udpsock + 1, &fds, NULL, NULL, NULL) == -1)
            perror("select"), abort();

        if (FD_ISSET(sock, &fds)) {
            struct sockaddr_in addr;
            u_int8_t type, code;
            u_int16_t id, seq;
            char *data = NULL;
            ssize_t len;

            if (!recv_ping(sock, &addr, &type, &code, &id, &seq, &data, &len))
                goto ignore_ping;

            if (type != ICMP_ECHO || code != 0 || seq != 0)
                goto ignore_ping;

            if (len < sizeof(long) || ntohl(* (long *) data) != key)
                goto ignore_ping;

            lastaddr = addr;
            id_pool[id_pool_w] = id;
            id_pool_w = (id_pool_w + 1) % POOLSZ;
            if (id_pool_w == id_pool_r)
                id_pool_r++;

            if (len == sizeof(long))
                goto ignore_ping;

            if (write(udpsock, data + sizeof(long), len - sizeof(long)) == -1)
                perror("write");

ignore_ping:
            free(data);
        }

        if (FD_ISSET(udpsock, &fds)) {
            char buffer[4096 + sizeof(long)];
            ssize_t len;

            * (long *) buffer = htonl(key);
            if ((len = read(udpsock, buffer + sizeof(long), 4096)) == -1) {
                perror("read"); goto ignore_udp_fail;
            }

            /* firewall:
             * -A OUTPUT -p icmp -s xx.xx.xx.xx -m icmp --icmp-type 0 -m u32 --u32 0x1a&0xffff0000=0x0 -j DROP
             */
            if (id_pool_r != id_pool_w) {
                send_ping(sock, &lastaddr, ICMP_ECHOREPLY, 0,
                        id_pool[id_pool_r], 1, buffer, len + sizeof(long));
                id_pool_r = (id_pool_r + 1) % POOLSZ;
            }
        }
ignore_udp_fail: continue;
    }

    return 0;
}
