/*
 * Copyright (c) 2014, Yiyu Lin <linyiyu1992 at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#include <stdlib.h>
#include <pthread.h>

#define BUFSIZE 10240
#define FOUR04 "HTTP/1.1 404 Not Found\nContent-Length: 219\nContent-Type: text/html\n\n<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><p>These aren't the bytes you're looking for.</p></body></html>"
#define FOUR00 "HTTP/1.1 400 Bad Request\nContent-Length: 230\nContent-Type: text/html\n\n<html><head><title>400 Bad Request</title></head><body><h1>400 Bad Request</h1><p>If this is an HTTP request, where is the host?</p></body></html>"

extern int     startserver();
void* serve_client(void* v);

typedef struct
{
    int sock;
    struct sockaddr_in sin_cli;
    socklen_t len;
} sock_connection_t;


// failure
void fail(const char* str)
{
    perror(str); exit(1);
}
void fail_thread(const char* str)
{
    perror(str);
    pthread_exit(0);
}

// parse request for host
char* parse_host(char* buf, char** saveptr) {
  char* tok;
  tok = strtok_r(buf, " \r\n", saveptr);
  while (tok != NULL) {
    if (strcmp(tok, "Host:")==0) {
      tok = strtok_r(NULL, " \r\n", saveptr); // find host
      return tok;
    }
    tok = strtok_r(NULL, " \r\n", saveptr);
  }
  return NULL;
}

void close_connections(struct addrinfo* host, int sock, FILE* f) {
  if (host!=NULL) freeaddrinfo(host);
  fclose(f);
  printf("\e[1;34mConnection closed on fdesc %d\n\e[0m", sock);
  close(sock);
  pthread_exit(0);
}

void* process(void*);

int main(int argc, char* argv[])
{
    int servsock;
    pthread_t thread;
    sock_connection_t * connection;

    /* check usage */
    if (argc != 1) {
        fprintf(stderr, "usage : %s\n", argv[0]);
        exit(1);
    }

    /* get ready to receive requests */
    servsock = startserver();
    if (servsock == -1) {
        perror("Error on starting server: ");
        exit(1);
    }

    while (1) {
        connection = (sock_connection_t *)malloc(sizeof(sock_connection_t));

        connection->sock = accept(servsock, (struct sockaddr*)&connection->sin_cli, &connection->len);
        if (connection->sock<0) {
            free(connection);
            continue;
        }
        pthread_create(&thread, 0, serve_client, &connection->sock);
        pthread_detach(thread);
    }
}

// serve each connection
void* serve_client(void* v) {
  int sock_client = *(int*) v;
  free(v);

  printf("ok\n");

  // fdesc for client socket
  FILE* client_r = fdopen(sock_client, "r");
  if (client_r==NULL) fail_thread("fdopen");
  setlinebuf(client_r);

  // obtain header
  char buf[BUFSIZE];
  char req[BUFSIZE];
  while (fgets(buf, BUFSIZE, client_r) != NULL) {
    strcat(req, buf);
    if (strlen(buf) < 3) break;
  }

  // find host and port
  char* svptr;
  char  req_cpy[BUFSIZE];
  strcpy(req_cpy, req);
  char* host_name = parse_host(req_cpy, &svptr);
  if (host_name==NULL) {
    int n = write(sock_client, FOUR00, strlen(FOUR00));
    if (n<0) fail_thread("write");
    close_connections(NULL, sock_client, client_r);
  }

  // dns stuff
  int err;
  struct addrinfo* host;
  err = getaddrinfo(host_name, "80", NULL, &host);
  if (err) {
    fprintf(stderr, "%s : %s\n", host_name, gai_strerror(err));
    int n = write(sock_client, FOUR04, strlen(FOUR04));
    if (n<0) fail_thread("write");
    close_connections(host, sock_client, client_r);
  }

  // make server socket
  int sock_server = socket(host->ai_family, SOCK_STREAM, 0);
  if (sock_server < 0) fail_thread("socket");

  // connect to server socket
  if (connect(sock_server, host->ai_addr, host->ai_addrlen)) {

    // failed, 404 and exit
    int n = write(sock_client, FOUR04, strlen(FOUR04));
    if (n<0) fail_thread("write");
    close_connections(host, sock_client, client_r);

  } else {

    // write header
    FILE* server_w = fdopen(sock_server, "w");
    if (server_w == NULL) fail_thread("fdopen");
    setlinebuf(server_w);
    printf("----------\n%s----------\n", req);
    fputs(req, server_w);

    // get response
    int n = read(sock_server, buf, BUFSIZE);
    while (n>0) {
      n = write(sock_client, buf, n);
      if (n<0) fail_thread("write");
      n = read(sock_server, buf, BUFSIZE);
    }
    if (n<0) fail_thread("read");
    fclose(server_w);
  }
  close(sock_server);
  close_connections(host, sock_client, client_r);
}
