/* **************************************************
 * wol.c - Simple Wake-On-LAN utility to wake a networked PC.
 * Author: Michael Sanders
 * Usage: wol [-q] [-b <bcast>] [-p <port>] <dest>
 * Compile it with: gcc -Wall -Os -DNDEBUG -o wol wol.c
 * Last updated: December 28, 2009
 *
 * LICENSE
 * --------------------------------------------------
 * Copyright (c) 2009-2010 Michael Sanders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * --------------------------------------------------
 *
 * ************************************************** */

#include <stdio.h> /* printf(), etc. */
#include <ctype.h> /* isdigit() */
#include <assert.h> /* assert() */
#include <unistd.h> /* getopt() */
#include <stdlib.h> /* exit() */
#include <string.h> /* memset() & memcpy() */
#include <arpa/inet.h> /* htonl() & htons() */
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h> /* socket(), sendto(), setsockopt() */
#include <stddef.h> /* size_t */
#include <limits.h> /* UINT_MAX */
#include <net/if.h>
#include <errno.h> /* errno */
#include <sys/ioctl.h>

#include "config.h"
#include "defines.h"

u_char outpack[1000];
u_char wol_passwd[6];
int wol_passwd_sz = 0;

static int get_fill(unsigned char *pkt, const struct cfgData *data);

int send_wol(const struct cfgData *data, char *ifname)
{
	struct sockaddr whereto;	/* who to wake up */
	int one = 1;				/* True, for socket options. */
	int s;						/* Raw socket */
	int i, pktsize;    
    /* Fill in the source address, if possible.
       The code to retrieve the local station address is Linux specific. */
    struct ifreq if_hwaddr;
#ifdef DEBUG_ALL
    unsigned char *hwaddr = (unsigned char *)if_hwaddr.ifr_hwaddr.sa_data;
#endif    
	/* Note: PF_INET, SOCK_DGRAM, IPPROTO_UDP would allow SIOCGIFHWADDR to
	   work as non-root, but we need SOCK_PACKET to specify the Ethernet
	   destination address. */
	if ((s = socket(AF_INET, SOCK_PACKET, SOCK_PACKET)) < 0) {
		if (errno == EPERM)
			fprintf(stderr, "ether-wake must run as root\n");
		else
			perror("ether-wake: socket");
	}
	/* Don't revert if debugging allows a normal user to get the raw socket. */
	if (setuid(getuid()) != 0)
    {
        fprintf(stderr,"Can not set UID\n");
    }

	pktsize = get_fill(outpack, data);
    strcpy(if_hwaddr.ifr_name, ifname);
    if (ioctl(s, SIOCGIFHWADDR, &if_hwaddr) < 0) {
        fprintf(stderr, "SIOCGIFHWADDR on %s failed: %s\n", ifname,
                strerror(errno));
        return 1;
    }
    memcpy(outpack+6, if_hwaddr.ifr_hwaddr.sa_data, 6);

    debug_printf("The hardware address (SIOCGIFHWADDR) of %s is type %d  "
            "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n", ifname,
            if_hwaddr.ifr_hwaddr.sa_family, hwaddr[0], hwaddr[1],
            hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

    if (wol_passwd_sz > 0) {
        memcpy(outpack+pktsize, wol_passwd, wol_passwd_sz);
        pktsize += wol_passwd_sz;
    }

#ifdef DEBUG_ALL
        debug_printf("The final packet is: ");
        for (i = 0; i < pktsize; i++)
            debug_printf(" %2.2x", outpack[i]);
        debug_printf(".\n");
#endif

    /* This is necessary for broadcasts to work */
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&one, sizeof(one)) < 0)
        perror("setsockopt: SO_BROADCAST");

    whereto.sa_family = 0;
    strcpy(whereto.sa_data, ifname);

    if ((i = sendto(s, outpack, pktsize, 0, &whereto, sizeof(whereto))) < 0)
        perror("sendto");
    else 
        debug_printf("Sendto worked ! %d.\n", i);
    close(s); 
    return 0;

}

static int get_fill(unsigned char *pkt, const struct cfgData *data)
{
	int offset, i;

	memcpy(pkt, data->ether, 6);
	memcpy(pkt+6, data->ether, 6);
	pkt[12] = 0x08;				/* Or 0x0806 for ARP, 0x8035 for RARP */
	pkt[13] = 0x42;
	offset = 14;

	memset(pkt+offset, 0xff, 6);
	offset += 6;

	for (i = 0; i < 16; i++) {
		memcpy(pkt+offset, data->ether, 6);
		offset += 6;
	}
#ifdef DEBUG_ALL
		debug_printf("Packet is ");
		for (i = 0; i < offset; i++)
			debug_printf(" %2.2x", pkt[i]);
		debug_printf(".\n");
#endif        
	return offset;
}
