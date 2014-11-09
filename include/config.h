/* config.h -- header for config.c

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <netinet/in.h>
#include <net/ethernet.h>
#include <pthread.h>

typedef enum
{
    tUnknown=0,
    tGoingUp,
    tUp,
    tGoingDown,
    tDown,
}ipstatus;

struct cfgData {
    char section[30];
    int id;
    struct in_addr ip;
    unsigned char ether[ETHER_ADDR_LEN];
    unsigned int *tcpportarray;
    size_t tcpportarraylen;
    unsigned int *udpportarray;
    size_t udpportarraylen;
    ipstatus status;
    int *sockets;
    int *acceptsockets;
    //u_char *forwardpacket;
    //int fwpacketlen;
    pthread_mutex_t accessmutex;
};

struct hostData
{
    struct cfgData data;
    struct hostData *next;
};
extern struct hostData *hostBuffer;

int read_config(char *cfgName);
struct cfgData *get_cfg(struct in_addr ipaddr);
void destroy_config();
int set_ifaces(const char *dev);
int ifdown(const char *dev, struct cfgData *data);
int ifup(const char *dev, struct cfgData *data);
size_t get_configsize();
char *get_filters();
#endif
