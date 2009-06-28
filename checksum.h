/* 
 * simple ICMP tunnel
 * vim:set sw=4 sta et:
 *
 * Copyright (C) 2009  Tomas Janousek <tomi@nomi.cz>
 */
#ifndef CHECKSUM_H_INCLUDED
#define CHECKSUM_H_INCLUDED
#include <netinet/in.h>
__BEGIN_DECLS
u_int16_t checksum(unsigned char *addr, int count);
__END_DECLS
#endif /* CHECKSUM_H_INCLUDED */
