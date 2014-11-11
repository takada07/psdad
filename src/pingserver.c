#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "ping.h"
#include "sniffer.h"
#include "defines.h"
#include "err.h"

int server_running;
pthread_t *ping_threads;
pthread_t serverthread;

char *dev;
void *ping_thread(void *arg)
{
    if (arg != NULL)
    {
        struct cfgData *data = (struct cfgData *)arg;
        if (data->status == tUp)
        {
            if (ping(&data->ip) != 0)
            {
                //ip is down, turning virtual iface up;
                pthread_mutex_lock(&data->accessmutex);
                ifup(dev,data);
                debug_printf("Device is down, should going up\n");
                data->status=tDown;
                pthread_mutex_unlock(&data->accessmutex);
            }
        }
        else if (data->status == tGoingUp)
        {
            debug_printf("Going up\n");
            if (ping(&data->ip) == 0)
            {
                debug_printf("Going up id UP\n");
                pthread_mutex_lock(&data->accessmutex);
                    //inject_packet(data);
                data->status=tUp;
                pthread_mutex_unlock(&data->accessmutex);
            }
        }
        else if (data->status==tUnknown)
        {
            debug_printf("Unknown status of device\n");
            if (ping(&data->ip) == 0)
            {
                pthread_mutex_lock(&data->accessmutex);
//                inject_packet(data);
                data->status=tUp;
                pthread_mutex_unlock(&data->accessmutex);
            }
            else
            {
                //ip is down, turning virtual iface up;
                pthread_mutex_lock(&data->accessmutex);
                ifup(dev,data);
                //debug_printf("Device is down, should going up\n");
                data->status=tDown;
                pthread_mutex_unlock(&data->accessmutex);
            }

        }
    }
    return (NULL);
}

void *server_thread(void *arg)
{

    int i;
    while (server_running==0)
    {
        struct hostData *tmp = hostBuffer;        
        for(i=0;i<get_configsize();i++)
        {
            debug_printf("New ping cycle %i\n",get_configsize());
            pthread_create(&ping_threads[i],NULL,ping_thread,(void *)&tmp->data);
            pthread_join(ping_threads[i], NULL);
            tmp = tmp->next;
        }
        sleep(5);
    }
    return NULL;
}

int init_server(char *pdev)
{
    dev=pdev;
    int err;
    //create array of threads depending in amount of configured hosts
    ping_threads = (pthread_t*)malloc(get_configsize()*sizeof(pthread_t));
    //set server tu running state;
    server_running=0;
    err = pthread_create(&serverthread, NULL, &server_thread, NULL);
    if (err != 0)
    {
        return ERR_THREAD_CREATE;
    }
    return 0;
}
