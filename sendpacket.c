#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "defines.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) 
    {
       fprintf(stderr,"usage %s [-s/-w] hostname\n", argv[0]);
       exit(0);
    }
    int cont = 0;
    while (cont == 0)
    {
        FILE *in = fopen("/sys/class/net/eth0/operstate","r");
        char *line = NULL;
        size_t len;
        ssize_t read;
        if ((read = getline(&line, &len, in)) < 0)
            break;
        debug_printf("%s",line);
        if (strcmp(line,"up\n")==0)
                cont=1;
        fclose(in);
        sleep(1);
    }
    debug_printf("IFACE is up\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[2]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    debug_printf("We have host\n");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(SERVER_PORT);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    debug_printf("connected\n");
    bzero(buffer,256);
    if (strcmp(argv[1],"-w"))
    {
        //send I'm up message
        debug_printf("WAKEUP\n");
        strcpy(buffer,WAKEUP_MESSAGE);
    }
    else if (strcmp(argv[1],"-s"))
    {
        debug_printf("SUSPEND\n");
        strcpy(buffer,SUSPEND_MESSAGE);
    }
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    close(sockfd);
    return 0;
}
