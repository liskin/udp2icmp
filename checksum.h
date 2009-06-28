/* 
 * Relay Network monitor
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */
#ifndef CHECKSUM_H_INCLUDED
#define CHECKSUM_H_INCLUDED
#include <netinet/in.h>
__BEGIN_DECLS
u_int16_t checksum(unsigned char *addr, int count);
__END_DECLS
#endif /* CHECKSUM_H_INCLUDED */
