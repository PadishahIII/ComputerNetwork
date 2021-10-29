#pragma once

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
//#include "HttpHeader.h"
#include "HttpParser.h"
#include <arpa/inet.h>
#include <string.h>
#include <regex>
#include <sys/ioctl.h>
#include </usr/include/net/if.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <fcntl.h>
#include <set>
#include <netdb.h>
using namespace std;

struct HttpHeader
{
    char method[4];         // POST or GET，(no CONNECT)
    char url[1024];         // host url
    char host[1024];        // host
    char cookie[1024 * 10]; //cookie
    HttpHeader()
    {
        memset(this, 0, sizeof(HttpHeader));
    }
};

void ParseHttpHeader(char *, HttpHeader *, char *);
class server
{
public:
    int HTTP_PORT = 80;
    int MAXSIZE = 65507;

    threadpool *tpl; //线程池

    int ProxyServerFd; //代理服务器的文件描述符
    struct sockaddr_in ProxyServerAddr;
    int ProxyServerPort = 8888; //代理服务器端口

    int epollFd;                          //epoll实例
    struct epoll_event epollEvents[1024]; //要监听的文件描述符

    set<string> BanList = {
        //"error.baidu.com",
        "functional.events.data.microsoft.com:443",
        ""};
    set<string> PassList = {
        "error.baidu.com",
        "baidu.com",
        ""};

    server(int ProxyServerPort);
    server();
    void InitSocket(); //配置 socket bind listen epollinit
    HttpParser ParseHttpHead(char *buffer);
    int ConnectToServer(char *host, int port); //代理服务器充当客户端访问其它服务器
    void HandleHttpRequest(char *buffer, int clientFd);
    void Start();
    void closeFd(int);

    static void xx(){};
};

#endif