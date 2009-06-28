/* 
 * Relay Network monitor
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */
#include "checksum.h"

u_int16_t checksum(unsigned char *addr, int count)
{
    register long sum = 0;

    while( count > 1 )  
    {
	/*  This is the inner loop */
	sum += * ((u_int16_t*) addr);
	addr += sizeof(u_int16_t);
	count -= 2;
    }  
    /*  Add left-over byte, if any */
    if( count > 0 )
	sum += * (u_int8_t*) addr;
    /*  Fold 32-bit sum to 16 bits */
    while (sum>>16)
	sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}

