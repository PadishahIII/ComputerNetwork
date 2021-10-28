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
#include "HttpHeader.h"
#include <arpa/inet.h>
#include <string.h>
#include <regex>

class server
{
public:
    int HTTP_PORT = 80;
    int MAXSIZE = 65507;

    threadpool tpl{10}; //线程池

    int ProxyServerFd; //代理服务器的文件描述符
    struct sockaddr_in ProxyServerAddr;
    int ProxyServerPort = 8888; //代理服务器端口

    int epollFd;                          //epoll实例
    struct epoll_event epollEvents[1024]; //要监听的文件描述符

    server(int ProxyServerPort);
    void InitSocket(); //配置
    HttpHeader ParseHttpHead(char *buffer);
    int ConnectToServer(char *host, int port); //代理服务器充当客户端访问其它服务器
    void Start();
};

#endif