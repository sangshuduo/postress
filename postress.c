/*
 * Copyright (c) 2021 Shuduo Sang <sangshuduo@gmail.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the Apache License, version 2.0
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define REQ_BUF_LEN     1024
#define REP_BUF_LEN     4096

void ERROR(const char *msg) { perror(msg); exit(0); }

void HELP() {
    char indent[10] = "        ";
    printf("\n%s\n\n", 
        "Usage: ./postress -h <hostname> -p <port> -r <rest url> -u <username> -P <password> -a <auth str> -s <command>");
    printf("%s%s\t%s\n", indent, "-h", "specify the host to connect. Default is 127.0.0.1.");
    printf("%s%s\t%s\n", indent, "-p", "specify The TCP/IP port number to use for the connection. Default is 80.");
    printf("%s%s\t%s\n", indent, "-r", "specify the RESTful API url to use.");
    printf("%s%s\t%s\n", indent, "-u", "specify the user's name to use when connecting to the server.");
    printf("%s%s\t%s\n", indent, "-P", "specify the password to use when connecting to the server.");
    printf("%s%s\t%s\n", indent, "-a", "authorization string. instead of input user's name and password");
    printf("%s%s\t%s\n", indent, "-s", "sql command to execute.");
}

int main(int argc,char *argv[])
{
    char *host = "127.0.0.1";
    int port = 80;
    char *url = "";
    char *user;
    char *pass;
    char *auth;
    char *cmd;

    if (argc == 1) {
        HELP();
        exit(0);
    }

    int opt = 0;

    while ((opt = getopt(argc, argv, "h:p:r:u:P:a:s:")) != -1) {
        switch(opt) {
        case 'h':
            host = optarg; 
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'r':
            url = optarg;
            break;
        case 'u':
            user = optarg;
            break;
        case 'P':
            pass = optarg;
            break;
        case 'a':
            auth = optarg;
            break;
        case 's':
            cmd = optarg;
            break;
        case '?':
            if (optopt == 'h') {
                printf("\nPlease input host name to connect");
            } else if (optopt == 'p') {
                printf("\nPlease input port to connect");
            }
            break;
        default:
            HELP();
            exit(0);
        }
    }

    char *req_fmt = "POST %s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: postress/0.1\r\nAccept: */*\r\n%s\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n%s";

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total;
    char request[REQ_BUF_LEN], response[REP_BUF_LEN];

    int r = snprintf(request, REQ_BUF_LEN, req_fmt, url, host, port, auth, strlen(cmd), cmd);
    if (r >= REQ_BUF_LEN)
        ERROR("ERROR too long request");
    printf("Request:\n%s\n", request);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) ERROR("ERROR opening socket");

    server = gethostbyname(host);
    if (server == NULL) ERROR("ERROR, no such host");

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        ERROR("ERROR connecting");

    total = strlen(request);
    sent = 0;
    do {
        bytes = write(sockfd, request + sent, total - sent);
        if (bytes < 0)
            ERROR("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    memset(response, 0, sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    do {
        bytes = read(sockfd, response + received, total - received);
        if (bytes < 0)
            ERROR("ERROR reading response from socket");
        if (bytes == 0)
            break;
        received+=bytes;
    } while (received < total);

    if (received == total)
        ERROR("ERROR storing complete response from socket");

    close(sockfd);
    printf("Response:\n%s\n",response);

    return 0;
}

