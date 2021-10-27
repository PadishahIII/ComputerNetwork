#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include "threadpool.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

class server
{
public:
    int HTTP_PORT = 80;
    int MAXSIZE = 65507;

    threadpool tpl{10}; //线程池

    int ProxyServerFd; //代理服务器的文件描述符
    struct sockaddr_in ProxyServerAddr;
    int ProxyServerPort = 8888; //代理服务器端口

    server(int ProxyServerPort);
    void InitSocket();
    void ParseHttpHead(char *buffer, HttpHeader *httpHeader);
    bool ConnectToServer(struct sockaddr_in serverSocket, char *host);
    void Start();
};

#endif