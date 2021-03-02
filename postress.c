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
#include <string.h>

#ifdef WINDOWS
    #include <winsock2.h>
    #pragma comment ( lib, "ws2_32.lib" )
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
#endif


#define REQ_BUF_LEN     1024
#define REP_BUF_LEN     4096

void ERROR_EXIT(const char *msg) { perror(msg); exit(0); }

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

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            host = argv[++i]; 
        } else if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-r") == 0) {
            url = argv[++i];
        } else if (strcmp(argv[i], "-u") == 0) {
            user = argv[++i];
        } else if (strcmp(argv[i], "-P") == 0) {
            pass = argv[++i];
        } else if (strcmp(argv[i], "-a") == 0) {
            auth = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0) {
            cmd = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            HELP();
            exit(0);
        }
    }

#ifdef WINDOWS
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 1), &wsaData);
    SOCKET sockfd;
#else
    int sockfd;
#endif
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) ERROR_EXIT("ERROR opening socket");

    struct sockaddr_in serv_addr;

    struct hostent *server;
    server = gethostbyname(host);
    if (server == NULL) ERROR_EXIT("ERROR, no such host");

    printf("h_name: %s\nh_addretype: %s\nh_length: %h\n", 
            server->h_name, (server->h_addrtype == AF_INET)?"ipv4":"ipv6", server->h_length);

    memset(&serv_addr,0,sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
#ifdef WINDOWS
    serv_addr.sin_addr.s_addr = inet_addr(host);
#else
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
#endif

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        ERROR_EXIT("ERROR connecting");


    char *req_fmt = "POST %s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: postress/0.1\r\nAccept: */*\r\n%s\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n%s";

    int bytes, sent, received, total;
    char request[REQ_BUF_LEN], response[REP_BUF_LEN];

    int r = snprintf(request, REQ_BUF_LEN, req_fmt, url, host, port, auth, strlen(cmd), cmd);
    if (r >= REQ_BUF_LEN)
        ERROR_EXIT("ERROR too long request");

    printf("Request:\n%s\n", request);
    total = strlen(request);
    sent = 0;
    do {
#ifdef WINDOWS
        bytes = send(sockfd, request + sent, total - sent, 0);
#else
        bytes = write(sockfd, request + sent, total - sent);
#endif
        if (bytes < 0)
            ERROR_EXIT("ERROR writing message to socket");

        if (bytes == 0)
            break;

        sent += bytes;
    } while (sent < total);

    memset(response, 0, sizeof(response));
    total = sizeof(response)-1;
    received = 0;

    do {
#ifdef WINDOWS
        bytes = recv(sockfd, response + received, total - received, 0);
#else
        bytes = read(sockfd, response + received, total - received);
#endif
        if (bytes < 0)
            ERROR_EXIT("ERROR reading response from socket");

        if (bytes == 0)
            break;
        received += bytes;
    } while (received < total);

    if (received == total)
        ERROR_EXIT("ERROR storing complete response from socket");

    printf("Response:\n%s\n", response);

#ifdef WINDOWS
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return 0;
}

