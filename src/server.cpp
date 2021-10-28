#include "server.h"
using namespace std;

server::server(int ProxyServerPort)
{
    this->ProxyServerPort = ProxyServerPort;
}
void server::InitSocket()
{
    this->ProxyServerFd = socket(PF_INET, SOCK_STREAM, 0);
    this->ProxyServerAddr.sin_port = htons(this->ProxyServerPort);
    this->ProxyServerAddr.sin_family = AF_INET;
    this->ProxyServerAddr.sin_addr.s_addr = INADDR_ANY;

    bind(this->ProxyServerFd, (struct sockaddr *)&this->ProxyServerAddr, sizeof(this->ProxyServerAddr));
    listen(this->ProxyServerFd, 8);

    //epoll init
    this->epollFd = epoll_create(100);
    struct epoll_event epev;
    epev.events = EPOLLIN;
    epev.data.fd = this->ProxyServerFd; //监听代理服务器fd
}

/**
 * 连接服务器，返回与之通信的socket(fd)
 */
int server::ConnectToServer(char *url, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(-1);
    }
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, url, &serveraddr.sin_addr.s_addr);
    serveraddr.sin_port = htons(port);
    int re = connect(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

    if (re == -1)
    {
        perror("connect");
        exit(-1);
    }
    return fd;
}

//buffer:HTTP报文整体
HttpHeader server::ParseHttpHead(char *buffer)
{
    return HttpHeader(buffer);
}