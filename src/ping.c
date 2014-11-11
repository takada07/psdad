/* ping.c -- library for sending and receiving PING packet

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/

#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <strings.h>

#include "defines.h"
#include "err.h"

#define PACKETSIZE  64

struct packet
{
    struct icmphdr hdr;
    char msg[PACKETSIZE-sizeof(struct icmphdr)];
};

int pid=-1;
struct protoent *proto=NULL;
int cnt=1;

/*--------------------------------------------------------------------*/
/*--- checksum - standard 1s complement checksum                   ---*/
/*--------------------------------------------------------------------*/
unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

/*--------------------------------------------------------------------*/
/*--- ping - Create message and send it.                           ---*/
/*    return 0 is ping Ok, return 1 is ping not OK.                ---*/
/*--------------------------------------------------------------------*/
int ping(const struct in_addr *dest_ip)
{
    const int val=255;
    int i, sd;
    struct packet pckt;
    struct sockaddr r_addr;
    proto = getprotobyname("ICMP");
    struct sockaddr_in addr_ping,*addr;    
    int loop;
    addr = &addr_ping;
    addr_ping.sin_family =AF_INET;
    addr_ping.sin_port = 0;
    addr_ping.sin_addr.s_addr = dest_ip->s_addr;
    pid = getpid();
    debug_printf("ping started %x\n", dest_ip->s_addr);
    sd = socket(AF_INET, SOCK_RAW, proto->p_proto);
    if ( sd < 0 )
    {
        return ERR_SOCKET_INIT_FAIL;
    }
    debug_printf("raw socket\n");
    if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
    {
        return ERR_OPT_TTL;
    }
    if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
    {
        return ERR_NONBLOCKING_IO;
    }
    debug_printf("Start ping loop\n");
    for (loop=0;loop < 10; loop++)
    {

        unsigned int len=sizeof(r_addr);

        if ( recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) > 0 )
        {
            struct sockaddr_in *addrin = (struct sockaddr_in *)&r_addr;
            if (addrin->sin_addr.s_addr == dest_ip->s_addr)
            {
                debug_printf("ping recv %x\n",dest_ip->s_addr);
                return ERR_OK;
            }
        }

        bzero(&pckt, sizeof(pckt));
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = pid;
        for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
            pckt.msg[i] = i+'0';
        pckt.msg[i] = 0;
        pckt.hdr.un.echo.sequence = cnt++;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
        if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
            return ERR_SENDTO;
        usleep(300000);
    }

    return ERR_PING;
}

