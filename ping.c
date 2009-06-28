/* 
 * simple ICMP tunnel
 * vim:set sw=4 sta et:
 *
 * Copyright (C) 2009  Tomas Janousek <tomi@nomi.cz>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "checksum.h"
#include "ping.h"

void resolv(const char *host, struct sockaddr_in* addr)
{
    struct hostent *he;

    if ((he = gethostbyname(host)) == NULL)
        fprintf(stderr, "gethostbyname: %s\n", hstrerror(h_errno)), abort();

    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    memcpy(&addr->sin_addr, he->h_addr, he->h_length);
}

int init_ping_socket(void)
{
    int sock;

    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
        perror("socket"), abort();

    int ttl = 255;
    setsockopt(sock, SOL_IP, IP_TTL, (const char *)&ttl, sizeof(ttl));

    return sock;
}

void bind_ping_socket(int sock, struct sockaddr_in *addr)
{
    if (bind(sock, (const struct sockaddr *) addr,
                sizeof(struct sockaddr_in)) == -1)
        perror("bind"), abort();
}

void send_ping(int sock, struct sockaddr_in *addr,
        u_int16_t id, u_int16_t seq, char *data, size_t len)
{
    struct icmphdr *p = (struct icmphdr*) alloca(sizeof(struct icmphdr) + len);

    p->type = ICMP_ECHO;
    p->code = 0;
    p->un.echo.id = htons(id);
    p->un.echo.sequence = htons(seq);
    p->checksum = 0;

    // make some data
    memcpy(((char *) p) + sizeof(struct icmphdr), data, len);

    // checksum it
    p->checksum = checksum((unsigned char*) p, sizeof(struct icmphdr) + len);

    if (sendto(sock, (char *) p, sizeof(struct icmphdr) + len, 0,
            (struct sockaddr *) addr, (socklen_t) sizeof(struct sockaddr_in)) == -1)
        perror("sendto"), abort();
}

int recv_ping(int sock, struct sockaddr_in* addr,
        u_int16_t *id, u_int16_t *seq, char** data, ssize_t *len)
{
    struct iphdr *buffer = (struct iphdr *) alloca(4096);
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if ((*len = recvfrom(sock, (char*) buffer, 4096, 0,
            (struct sockaddr *) addr, &addrlen)) == -1)
        perror("recvfrom"), abort();

    if (*len < sizeof(struct iphdr))
        return 0;

    struct icmphdr *p = (struct icmphdr*) ((char *) buffer + buffer->ihl * 4);
    *len -= ((char *) p) - ((char *) buffer) + sizeof(struct icmphdr);

    if (*len <= 0)
        return 0;

    if (p->type != ICMP_ECHO)
        return 0;

    *id = ntohs(p->un.echo.id);
    *seq = ntohs(p->un.echo.sequence);

    *data = (char *) malloc(*len);
    if (!*data)
        perror("malloc"), abort();

    memcpy(*data, ((char *) p) + sizeof(struct icmphdr), *len);

    return 1;
}
