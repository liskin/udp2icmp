/* 
 * simple ICMP tunnel
 * vim:set sw=4 sta et:
 *
 * Copyright (C) 2009  Tomas Janousek <tomi@nomi.cz>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include "ping.h"

/* To filter out kernel's ECHO replies, add this to iptables:
 * -A OUTPUT -p icmp -s xx.xx.xx.xx -m icmp --icmp-type 0 -m ttl --ttl-lt 100 -j DROP
 */

int init_udp_socket(unsigned short port, int server)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        perror("socket"), abort();

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if ((server ? connect : bind)(sock, (const struct sockaddr *) &addr,
                sizeof(struct sockaddr_in)) == -1)
        perror(server ? "connect" : "bind"), abort();

    return sock;
}

int main(int argc, char *argv[])
{
    char *progname = basename(argv[0]);
    int server;

    if (strcmp(progname, "udp2icmpsrv") == 0)
        server = 1;
    else if (strcmp(progname, "udp2icmpcli") == 0)
        server = 0;
    else
        fprintf(stderr, "run this as either udp2icmpsrv or udp2icmpcli\n"),
        exit(1);

    if (argc != 4)
        fprintf(stderr, "%s port host key\n", progname), exit(1);

    int port = atoi(argv[1]);
    struct sockaddr_in host; resolv(argv[2], &host);
    long key = atol(argv[3]);

    int sock = init_ping_socket();
    if (server)
        bind_ping_socket(sock, &host);
    int udpsock = init_udp_socket(port, server);

    struct sockaddr_in lastaddr;
    int lastaddr_valid = 0; // client
#define POOLSZ 16
    u_int16_t id_pool[POOLSZ]; // server
    int id_pool_r = 0, id_pool_w = 0;

    int nfds;
    fd_set fds;
    struct timeval tv;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        FD_SET(udpsock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        if ((nfds = select(udpsock + 1, &fds, NULL, NULL,
                        server ? NULL : &tv)) == -1)
            perror("select"), abort();

        if (FD_ISSET(sock, &fds)) {
            struct sockaddr_in addr;
            u_int8_t type, code;
            u_int16_t id, seq;
            char *data = NULL;
            ssize_t len;

            if (!recv_ping(sock, &addr, &type, &code, &id, &seq, &data, &len))
                goto ignore_ping;

            // is it the right ICMP packet?
            if (type != (server ? ICMP_ECHO : ICMP_ECHOREPLY)
                    || code != 0 || seq != 0)
                goto ignore_ping;

            // check the key
            if (len < sizeof(long) || ntohl(* (long *) data) != key)
                goto ignore_ping;

            if (server) {
                // add the ICMP id to the pool
                lastaddr = addr;
                id_pool[id_pool_w] = id;
                id_pool_w = (id_pool_w + 1) % POOLSZ;
                if (id_pool_w == id_pool_r)
                    id_pool_r++;
            }

            // drop empty packets, drop packets we don't know where to send
            if (len == sizeof(long) || (!server && !lastaddr_valid))
                goto ignore_ping;

            // if reached, send the packet
            if (sendto(udpsock, data + sizeof(long), len - sizeof(long), 0,
                    server ? NULL : (struct sockaddr *) &lastaddr,
                    server ? 0 : sizeof(struct sockaddr_in)) == -1)
                perror("sendto");

ignore_ping:
            free(data);
        }

        if (FD_ISSET(udpsock, &fds)) {
            char buffer[4096 + sizeof(long)];
            socklen_t addrlen = sizeof(struct sockaddr_in);
            ssize_t len;

            * (long *) buffer = htonl(key);
            if ((len = recvfrom(udpsock, buffer + sizeof(long), 4096, 0,
                    server ? NULL : (struct sockaddr *) &lastaddr,
                    server ? 0 : &addrlen)) == -1) {
                perror("recvfrom"); goto ignore_udp_fail;
            }

            // choose an ICMP id
            u_int16_t id;
            if (server) {
                if (id_pool_r != id_pool_w) {
                    id = id_pool[id_pool_r];
                    id_pool_r = (id_pool_r + 1) % POOLSZ;
                } else
                    // pool exhausted
                    goto ignore_udp_fail;
            } else
                id = rand();

            // send the ICMP packet
            send_ping(sock, &host, server ? ICMP_ECHOREPLY : ICMP_ECHO, 0,
                    id, 0, buffer, len + sizeof(long));

            // remember the address for UDP packets
            if (!server)
                lastaddr_valid = 1;
        }
ignore_udp_fail:

        // client sends periodic ICMP ECHO requests to feed the pool
        if (!server && nfds == 0) {
            long xkey = htonl(key);

            u_int16_t x = rand();
            send_ping(sock, &host, ICMP_ECHO, 0,
                    x, 0, (char *) &xkey, sizeof(long));
        }
    }

    return 0;
}
