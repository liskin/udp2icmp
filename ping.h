/* 
 * simple ICMP tunnel
 * vim:set sw=4 sta et:
 *
 * Copyright (C) 2009  Tomas Janousek <tomi@nomi.cz>
 */
#ifndef PING_H_INCLUDED
#define PING_H_INCLUDED

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

void resolv(const char *host, struct sockaddr_in* addr);
int init_ping_socket(void);
void bind_ping_socket(int sock, struct sockaddr_in *addr);
void send_ping(int sock, struct sockaddr_in *addr,
        u_int16_t id, u_int16_t seq, char *data, size_t len);
int recv_ping(int sock, struct sockaddr_in* addr,
        u_int16_t *id, u_int16_t *seq, char** data, ssize_t *len);

#endif /* PING_H_INCLUDED */
