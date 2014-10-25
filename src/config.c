/* config.c -- configuration library for psdad project

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "defines.h"
#include "config.h"
#include "err.h"
#include "strutil.h"
#include "ini.h"
#include "ping.h"

struct hostData
{
    struct cfgData data;
    struct hostData *next;
};

struct hostData *hostBuffer;
struct hostData *hostBufferLast;

struct cfgData *set_section(const char *section)
{
    struct hostData *tmp = hostBuffer; 
    while(tmp != NULL)
    {
        if (strcmp(tmp->data.section,section)==0)
            return (&tmp->data);
        else
            tmp = tmp->next;
    }
    tmp = (struct hostData *)malloc(sizeof(struct hostData));
    strcpy(tmp->data.section,section);
    tmp->data.forwardpacket=NULL;
    tmp->data.fwpacketlen=0;
    tmp->next=NULL;
    if (hostBuffer==NULL)
    {
        debug_printf("first item in config file\n");
        hostBuffer=tmp;
        hostBufferLast=tmp;
    }
    else
    {
        hostBufferLast->next=tmp;
        hostBufferLast=tmp;
    }  
    return(&tmp->data);
}

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    struct cfgData *data = set_section(section); 
    debug_printf("%s::%s::%s\n",section,name,value);
    if (data != NULL)
    {
        if (strcmp(name,"id")==0)
        {
            data->id = atoi(value);
        }
        else if (strcmp(name,"ip")==0)
        {
            if (inet_pton(AF_INET, value, &(data->ip)) == -1)
            {
                debug_printf("Bad IP Format in CFG file\n");
                return (0);
            }
            debug_printf("IP:%x\n",data->ip.s_addr);
        }
        else if (strcmp(name,"ether")==0)
        {
            if (sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x", (int *)&data->ether[0], (int *)&data->ether[1],(int *) &data->ether[2], (int *)&data->ether[3], (int *)&data->ether[4], (int *)&data->ether[5])!=6)
                printf("Bad Mac address in config file\n");
        }
        else if (strcmp(name,"tcpports")==0)
        {
            int i;
            if (strcmp(value,"")==0)
            {
                data->tcpportarraylen=0;
                data->tcpportarray=NULL;
                return 1;
            }
            //Setup forwarded tcpports
            char **ports = str_split(value,',',&data->tcpportarraylen);
            data->tcpportarray = (unsigned int *)malloc(data->tcpportarraylen * sizeof(unsigned int));
            debug_printf("TCPPorts[%d]\n",(int)data->tcpportarraylen);
            for (i=0;i<data->tcpportarraylen;i++)
            {
                data->tcpportarray[i]=atoi(ports[i]);
                free (ports[i]);
                debug_printf("%d\n",data->tcpportarray[i]);
            }
            free (ports);
        }
        else if (strcmp(name,"udpports")==0)
        {
            int i;
            if (strcmp(value,"")==0)
            {
                data->udpportarraylen=0;
                data->udpportarray=NULL;
                return 1;
            }
            //Setup forwarded tcpports
            char **ports = str_split(value,',',&data->udpportarraylen);
            data->udpportarray = (unsigned int *)malloc(data->udpportarraylen * sizeof(unsigned int));
            debug_printf("UDPPorts[%d]\n",(int)data->udpportarraylen);
            for (i=0;i<data->udpportarraylen;i++)
            {
                data->udpportarray[i]=atoi(ports[i]);
                free (ports[i]);
                debug_printf("%d\n",data->udpportarray[i]);
            }
            free (ports);
        }        
    }
    else
    {
        debug_printf("Error creating section\n");
        return 0;
    }
    return 1;
}

struct cfgData *get_cfg(struct in_addr ipaddr)
{
    struct hostData *tmp;
    tmp = hostBuffer;
    if (tmp == NULL)
        debug_printf("No config?\n");
    while (tmp != NULL)
    {
        if (tmp->data.ip.s_addr == ipaddr.s_addr)
        {
            return &tmp->data;
        }
        tmp = tmp->next;
    }
    return NULL;
}

int read_config(char *cfgName)
{
    hostBuffer=NULL;
    hostBufferLast=NULL;
    char *filename;
    if (cfgName==NULL)
    {
        filename=(char *) malloc(21);
        strcpy(filename,"/etc/psdad/psdad.cfg");
    }
    else
    {
        filename=cfgName;
    }
    debug_printf("%s\n",filename);
    if (ini_parse(filename, handler, NULL) < 0) {
        printf("Can't load '%s'\n",filename);
        return 1;
    }
    return ERR_OK;
}

int set_ifaces(const char *dev)
{
    struct hostData *tmp = hostBuffer;
    while (tmp != NULL)
    {
        if (ping(&tmp->data.ip) != 0)
        {
            //set interface, host is not responding
            #ifdef DEBUG_ALL
                char strip[20];
                inet_ntop(AF_INET,(void *)&tmp->data.ip, strip, INET_ADDRSTRLEN);
                debug_printf("IP=%s not responding\n",strip);
            #endif
            ifup(dev, &tmp->data);
        }
        else
        {
            tmp->data.status=0;
        }
        tmp=tmp->next;
    }
    return (ERR_OK);
}

void destroy_config()
{
    struct hostData *tmp,*tmp2;
    tmp = hostBuffer;
    while (tmp != NULL)
    {
        tmp2=tmp->next;
        if (tmp->data.tcpportarray != NULL)
            free(tmp->data.tcpportarray);
        if (tmp->data.udpportarray != NULL)
            free(tmp->data.udpportarray);
        if (tmp->data.forwardpacket != NULL)
            free(tmp->data.forwardpacket);
        free (tmp);
        tmp=tmp2;
    }
}

int ifdown(const char *dev, struct cfgData *data)
{
    char cmd[100];
    sprintf(cmd,"scripts/stopiface.sh %s %d",dev,data->id);
    data->status=0;
    return (system(cmd));
}

int ifup(const char *dev, struct cfgData *data)
{
    char cmd[100];
    char strip[20];
    inet_ntop(AF_INET,(void *)&data->ip, strip, INET_ADDRSTRLEN);
    sprintf(cmd,"scripts/startiface.sh eth0 %d %s",data->id,strip);
    data->status=1;
    return (system(cmd));
}

