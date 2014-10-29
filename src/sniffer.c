/* sniffer.c -- utils for reading data from pcap library

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/


#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */

#include "sniffer.h"
#include "err.h"
#include "defines.h"
#include "config.h"
#include "wake.h"

/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
        u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* dont fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};

typedef u_int tcp_seq;

struct sniff_tcp {
        u_short th_sport;               /* source port */
        u_short th_dport;               /* destination port */
        tcp_seq th_seq;                 /* sequence number */
        tcp_seq th_ack;                 /* acknowledgement number */
        u_char  th_offx2;               /* data offset, rsvd */
        #define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
        u_char  th_flags;
        #define TH_FIN  0x01
        #define TH_SYN  0x02
        #define TH_RST  0x04
        #define TH_PUSH 0x08
        #define TH_ACK  0x10
        #define TH_URG  0x20
        #define TH_ECE  0x40
        #define TH_CWR  0x80
        #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
        u_short th_win;                 /* window */
        u_short th_sum;                 /* checksum */
        u_short th_urp;                 /* urgent pointer */
};


pcap_t *handle=NULL;				/* packet capture handle */
char *dev = NULL;		        	/* capture device name */
char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
struct bpf_program fp;			/* compiled filter program (expression) */
int terminate = 0;
int foundcount = 0;

void forward_packet(int size_ip, struct cfgData *configdata, int packet_length, const u_char *packet)
{
//    u_char *forwardpacket = (u_char *)malloc(packet_length);
//    memcpy(forwardpacket,packet,packet_length);
//    struct sniff_ethernet *ethernet = (struct sniff_ethernet *) (forwardpacket);
//    memcpy(ethernet->ether_dhost,configdata->ether,ETHER_ADDR_LEN);
    pthread_mutex_lock(&configdata->accessmutex);
    ifdown(dev,configdata);
    send_wol(configdata,dev);
    configdata->status=tGoingUp;
//    configdata->forwardpacket=forwardpacket;
//    configdata->fwpacketlen=packet_length;
    pthread_mutex_unlock(&configdata->accessmutex);
}

int inject_packet(struct cfgData *data)
{
    debug_printf("inject_packet\n");
    if(data->forwardpacket != NULL)
    {
        if (pcap_inject(handle,data->forwardpacket,data->fwpacketlen)==-1) 
        {
            debug_printf("can not inject packet\n");
            pcap_perror(handle,0);
            return 1;
        }
        debug_printf("injecting packet\n")
        data->fwpacketlen=0;
        free(data->forwardpacket);
        data->forwardpacket=NULL;
        return 0;
    }
    debug_printf("inject packet is NULL\n");
    return 1;
}

void process_tcppacket(int size_ip, struct cfgData *configdata,int packet_length, const u_char *packet)
{
    int counter;
	struct sniff_tcp* tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);

	int size_tcp = TH_OFF(tcp)*4;

	if (size_tcp < 20) {
		debug_printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
		return;
	}    
    int dstport =  ntohs(tcp->th_dport); 
	debug_printf("   Dst port: %d\n",dstport);    
    for (counter = 0; counter < configdata->tcpportarraylen; counter++)
    {
        if (dstport == configdata->tcpportarray[counter])
        {
            debug_printf("TCP port found, forwarding\n");
            forward_packet(size_ip, configdata, packet_length, packet);
        }
    }    
}

void got_packet(u_char *args, const struct pcap_pkthdr *hdr, const u_char *packet)
{
	const struct sniff_ip *ip;              /* The IP header */
	int size_ip;

	ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {
		printf("   * Invalid IP header length: %u bytes\n", size_ip);
		return;
	}
    struct cfgData *configdata;
    configdata = get_cfg(ip->ip_dst);
    if (configdata == NULL)
    {
        return;
    }
    debug_printf("Listened ip\n");
    if (configdata->status!=tDown)
        return;
	switch(ip->ip_p) {
		case IPPROTO_TCP:
			debug_printf("   Protocol: TCP\n");
            process_tcppacket(size_ip,configdata,hdr->len,packet);
			break;;
		case IPPROTO_UDP:
			printf("   Protocol: UDP\n");
			break;
		default:
			debug_printf("   Not supported protocol\n");
			break;
	}    
}


int init_sniffer(char *conf_dev)
{
	char filter_exp[] = "ip";		/* filter expression [3] */
	bpf_u_int32 mask;			/* subnet mask */
	bpf_u_int32 net;			/* ip */

	/* check for capture device name on command-line */
	if (conf_dev != NULL) 
    {
        dev = conf_dev;
	}
	else 
    {
		/* find a capture device if not specified on command-line */
		dev = pcap_lookupdev(errbuf);
		if (dev == NULL) {
            return (ERR_DEFAULT_DEV_NOT_FOUND);
		}
	}
    debug_printf("%s\n",dev);
	/* get network number and mask associated with capture device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		net = 0;
		mask = 0;        
        return (ERR_NETMASK_DEV); 
	}

	/* print capture info */
    #ifdef DEBUG_ALL
	    debug_printf("Device: %s\n", dev);
    	debug_printf("Filter expression: %s\n", filter_exp);
    #endif
	/* open capture device */
	handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);
	if (handle == NULL) {
        return (ERR_DEVICE_OPEN);
	}

	/* make sure we're capturing on an Ethernet device [2] */
	if (pcap_datalink(handle) != DLT_EN10MB) {
        return (ERR_DEV_NOT_ETHER);
	}

	/* compile the filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
        return (ERR_PARSE_FILTER);
	}

	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
        return ERR_INSTALL_FILTER;
	}
    debug_printf("init_sniffer done\n");
    return (ERR_OK);
}

int start_loop()
{
	/* now we can set our callback function */
    while (terminate == 0)
    {
        pcap_loop(handle, 1, got_packet, NULL);
    }

	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(handle);

	printf("\nCapture complete.\n");

return 0;
    
}
