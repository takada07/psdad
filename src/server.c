/* server.c -- simple server for managing UP request for hosts

Copyright (c) <2014> Radoslav Strobl <spacok@gmail.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defines.h"
#include "config.h"
#include "sniffer.h"

/* port we're listening on */
#define PORT 2020
fd_set master;
int listener;
/* maximum file descriptor number */
struct fdconfig
{
    int fd;
    struct cfgData *data;
    struct fdconfig *next;
    struct fdconfig *prev;
};

struct fdconfig *iplist;
struct fdconfig *last;
int fdmax;

int remove_ip(int fd)
{
    struct fdconfig *tmp;
    struct fdconfig *prev;
    tmp=iplist;
    prev=NULL;
    while (tmp != NULL)
    {
        if (tmp->fd == fd)
        {
            if (prev != NULL)
                prev->next=tmp->next;
            else if (prev == NULL)
                iplist=tmp->next;
            if (tmp->next == NULL)
                last=prev;
            free(tmp);
            return 0;
        }
        prev=tmp;
        tmp=tmp->next;
    }
    return 1;
}

void add_ip(int fd, struct cfgData *data)
{
    struct fdconfig *tmp;
    tmp=malloc(sizeof(struct fdconfig));
    tmp->fd=fd;
    tmp->data=data;
    tmp->next=NULL;
    if (last == NULL)
    {
        iplist=tmp;
        last=tmp;
    }
    else
    {
        last->next=tmp;
        last=tmp;
    }

}

struct cfgData *find_ip(int fd)
{
    struct fdconfig *tmp = iplist;
    while (tmp != NULL)
    {
        if (tmp->fd == fd)
        {
            return tmp->data;
        }
        tmp = tmp->next;
    }
    return (NULL);
}

int check_server()
{
    /* client address */
    struct sockaddr_in clientaddr;
    /* temp file descriptor list for select() */
    fd_set read_fds;
    int i;
    FD_ZERO(&read_fds);
    int addrlen;
    int newfd;
    /* buffer for client data */
    char buf[1024];
    int nbytes;
    /* for setsockopt() SO_REUSEADDR, below */
    read_fds = master;
    struct cfgData *data;

    if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
    {
        perror("Server-select() error lol!");
        exit(1);
    }
    printf("Server-select() is OK...\n");

    /*run through the existing connections looking for data to be read*/
    for(i = 0; i <= fdmax; i++)
    {
        if(FD_ISSET(i, &read_fds))
        { /* we got one... */
            if(i == listener)
            {
                /* handle new connections */
                addrlen = sizeof(clientaddr);
                if((newfd = accept(listener, (struct sockaddr *)&clientaddr,(socklen_t *) &addrlen)) == -1)
                {
                    perror("Server-accept() error lol!");
                }
                else
                {
                    printf("Server-accept() is OK...\n");

                    FD_SET(newfd, &master); /* add to master set */
                    if(newfd > fdmax)
                    { /* keep track of the maximum */
                        fdmax = newfd;
                    }
                    struct sockaddr_in *addrin = (struct sockaddr_in *)&clientaddr;
                    data = get_cfg(addrin->sin_addr);
                    add_ip(newfd,data);

                }
            }
            else
            {
                /* handle data from a client */
                if((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0)
                {
                    /* got error or connection closed by client */
                    if(nbytes != 0)
                    {
                        /* connection closed */
                        debug_printf(" socket %d has data\n", i);
                        //inject packet
                        data = find_ip(i);
                        if (data != NULL)
                        {
                            if (strcmp(buf,"Here comes the magic")==0)
                            {
                                inject_packet(data);
                            }
                        }
                    }
                    else
                        perror("recv() error lol!");

                    /* close it... */
                    close(i);
                    /* remove from master set */
                    FD_CLR(i, &master);
                }
                else
                {
                }
            }
        }
    }
    return (0);
}

int init_server(const char *dev) 
{
    /* master file descriptor list */
    /* server address */
    struct sockaddr_in serveraddr;
    iplist=NULL;
    last=NULL;
    /* newly accept()ed socket descriptor */
    int yes = 1;
    /* clear the master and temp sets */
    FD_ZERO(&master);

    /* get the listener */
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Server-socket() error lol!");
        /*just exit lol!*/
        exit(1);
    }
    printf("Server-socket() is OK...\n");
    /*"address already in use" error message */
    if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("Server-setsockopt() error lol!");
        exit(1);
    }
    printf("Server-setsockopt() is OK...\n");

    /* bind */
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(PORT);
    memset(&(serveraddr.sin_zero), '\0', 8);

    if(bind(listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
    {
        perror("Server-bind() error lol!");
        exit(1);
    }
    printf("Server-bind() is OK...\n");

    /* listen */
    if(listen(listener, 10) == -1)
    {
        perror("Server-listen() error lol!");
        exit(1);
    }
    printf("Server-listen() is OK...\n");

    /* add the listener to the master set */
    FD_SET(listener, &master);
    /* keep track of the biggest file descriptor */
    fdmax = listener; /* so far, it's this one*/

    return 0;
}

