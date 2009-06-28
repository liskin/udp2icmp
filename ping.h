/* 
 * Relay Network monitor
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
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

int init_ping_socket(void);
void resolv(const char *host, struct sockaddr_in* addr);
void send_ping(int sock, struct sockaddr_in *addr,
	u_int16_t id, u_int16_t seq, char *data, size_t len);
int recv_ping(int sock, struct sockaddr_in* addr,
	u_int16_t *id, u_int16_t *seq, char** data, ssize_t *len);

#endif /* PING_H_INCLUDED */
