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
#include <map>
#include "epoller.h"

#define debug
using namespace std;

struct HttpHeader
{
    char method[4];         // POST or GET，(no CONNECT)
    char url[1024];         // host url
    char host[1024];        // host
    char cookie[1024 * 10]; //cookie
    int port;
    HttpHeader()
    {
        port = 80;
        memset(this, 0, sizeof(HttpHeader));
    }
};

void replace(char buffer_c[], const string &oldstr, const string &newstr);

class server
{
public:
    int HTTP_PORT = 80;
    int MAXSIZE = 65507;

    threadpool *tpl; //线程池

    int ProxyServerFd; //代理服务器的文件描述符
    struct sockaddr_in ProxyServerAddr;
    int ProxyServerPort = 8888; //代理服务器端口

    Epoller *epoller = new Epoller();

    set<string> BanList = {
        //"error.baidu.com",
        //"functional.events.data.microsoft.com:443",
        //"baidu.com",
        "c.urs.microsoft.com",
        ""};
    set<string> PassList = {
        "error.baidu.com",
        "baidu.com",
        ""};
    map<string, string> TransList = {
        {"", ""}};

    server(int ProxyServerPort);
    server();
    void InitSocket();                          //配置 socket bind listen epollinit
    int ConnectToServer(string host, int port); //代理服务器充当客户端访问其它服务器
    void HandleHttpRequest(char *, int, int);   //处理客户端请求
    bool ParseHttpHeader(char *, HttpHeader *, char *);
    void Start();
    void CloseConn(int);     //关闭fd
    void fcntlNonBlock(int); //设置非阻塞

    void DealListen();
    int DealRead(int, char *);

    int Recv(int, void *, size_t, int);
    int Send(int, const void *, size_t, int);
    int Select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

    void SendError(int, char *);
};

#endif