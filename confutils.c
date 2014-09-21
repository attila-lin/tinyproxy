/*
 * Copyright (c) 2014, Yiyu Lin <linyiyu1992 at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h> 
#include <errno.h>

#include <stdlib.h>

#define MAXNAMELEN 256

/* prepare server to accept requests
   returns file descriptor of socket
   returns -1 on error
*/
int startserver()
{
    int     sd;        /* socket descriptor */

    char *  servhost;  /* full name of this host */
    ushort  servport;  /* port assigned to this server */

    struct sockaddr_in sin_serv;
    struct hostent *hp;
    socklen_t len;
    char servaddr[100];

    if( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	return -1;

    bzero(&sin_serv, sizeof(sin_serv));
    sin_serv.sin_family = AF_INET;
    sin_serv.sin_port = htons(0);
    sin_serv.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(sd, (struct sockaddr *)&sin_serv, sizeof(sin_serv)) ){
	close(sd);
	return -1;
    }

    /* we are ready to receive connections */
    listen(sd, 5);

    servhost = malloc(MAXNAMELEN);
    if( gethostname(servhost, MAXNAMELEN) < 0 )
	return -1;
    if( (hp = gethostbyname(servhost)) < 0 )
	return -1;
    strcpy(servhost, hp->h_name);

    bzero(&sin_serv, sizeof(sin_serv));
    len = sizeof(sin_serv);
    if( getsockname(sd, (struct sockaddr *)&sin_serv, &len) < 0 ){
    	close(sd);
    	return -1;
    }
    servport = ntohs(sin_serv.sin_port);

    /* ready to accept requests */
    printf("admin: started server on '%s' at '%hu'\n",
	   servhost, servport);
    free(servhost);
    return(sd);
}

int readn(int sd, char *buf, int n)
{
    int     toberead;
    char *  ptr;

    toberead = n;
    ptr = buf;
    while (toberead > 0) {
	int byteread;

	byteread = read(sd, ptr, toberead);
	if (byteread <= 0) {
	    if (byteread == -1)
		perror("read");
	    return(0);
	}
    
	toberead -= byteread;
	ptr += byteread;
    }
    return(1);
}

