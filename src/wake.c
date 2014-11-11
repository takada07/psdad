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
#include "err.h"
//u_char wol_passwd[6];
//int wol_passwd_sz = 0;

static int get_fill(unsigned char *pkt, const struct cfgData *data);

int send_arping(const struct cfgData *data, char *ifname)
{
    u_char outpack[1000];
    struct sockaddr whereto;
    int sd;
	int one = 1;
    int offset = 0;
    if ((sd == socket(AF_INET, SOCK_PACKET, SOCK_PACKET)) < 0) 
    {
        if (errno == EPERM)
            return ERR_PERMISION;
        return ERR_SOCKET_INIT_FAIL;
    }
	if (setuid(getuid()) != 0)
    {
        return ERR_UID;
    }
    struct ifreq if_hwaddr;
    //broadcasts destination
    unsigned long destaddr = 0xFFFFFFFFFFFF;
    memcpy(outpack,&destaddr,6);
    offset += 6;
    strcpy(if_hwaddr.ifr_name, ifname);
    if (ioctl(sd, SIOCGIFHWADDR, &if_hwaddr) < 0) {
        return ERR_SIOCGIFHWADDR;
    }
    memcpy(outpack+offset, if_hwaddr.ifr_hwaddr.sa_data, 6);
    offset += 6;
    //Add ARP protocol
    unsigned int protocol = 0x0806;
    memcpy(outpack+offset, &protocol,2);
    offset += 2;
    unsigned int hwtype =0x0001;    //ETHERNET
    unsigned int protype = 0x0800;  //IP
    unsigned int opcode = 0x0001;
    memcpy(outpack+offset, &hwtype,2);
    offset += 2;
    memcpy(outpack+offset, &protype, 2);
    offset += 2;
    outpack[offset]=0x06;
    outpack[offset+1]=0x04;
    offset += 2;
    memcpy(outpack+offset,&opcode,2);
    offset += 2;
    memcpy(outpack+offset, if_hwaddr.ifr_hwaddr.sa_data, 6);
    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char *)&one, sizeof(one)) < 0)
        return ERR_SO_BROADCAST;
    offset += 6; 
    whereto.sa_family = 0;
    strcpy(whereto.sa_data, ifname);
    int i;
    if ((i = sendto(sd, outpack, offset, 0, &whereto, sizeof(whereto))) < 0)
        return ERR_SENDTO;
    else 
        debug_printf("Sendto worked ! %d.\n", i);
    close(sd);
    return ERR_OK;

}

int send_wol(const struct cfgData *data, char *ifname)
{
    u_char outpack[1000];
    struct sockaddr whereto;	/* who to wake up */
	int one = 1;				/* True, for socket options. */
	int s;						/* Raw socket */
	int i, pktsize;    
    /* Fill in the source address, if possible.
       The code to retrieve the local station address is Linux specific. */
    struct ifreq if_hwaddr;
	/* Note: PF_INET, SOCK_DGRAM, IPPROTO_UDP would allow SIOCGIFHWADDR to
	   work as non-root, but we need SOCK_PACKET to specify the Ethernet
	   destination address. */
	if ((s = socket(AF_INET, SOCK_PACKET, SOCK_PACKET)) < 0) 
    {
		if (errno == EPERM)
            return ERR_PERMISION;
		else
            return ERR_SOCKET_INIT_FAIL;
	}
	/* Don't revert if debugging allows a normal user to get the raw socket. */
	if (setuid(getuid()) != 0)
    {
        return ERR_UID;
    }

	pktsize = get_fill(outpack, data);
    strcpy(if_hwaddr.ifr_name, ifname);
    if (ioctl(s, SIOCGIFHWADDR, &if_hwaddr) < 0) {
        return ERR_SIOCGIFHWADDR;
    }
    memcpy(outpack+6, if_hwaddr.ifr_hwaddr.sa_data, 6);


    /* This is necessary for broadcasts to work */
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&one, sizeof(one)) < 0)
        return ERR_SO_BROADCAST;

    whereto.sa_family = 0;
    strcpy(whereto.sa_data, ifname);

    if ((i = sendto(s, outpack, pktsize, 0, &whereto, sizeof(whereto))) < 0)
        return ERR_SENDTO;
    else 
        debug_printf("Sendto worked ! %d.\n", i);
    close(s);
    sleep(3);
    return (send_arping(data,ifname));

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
	return offset;
}
