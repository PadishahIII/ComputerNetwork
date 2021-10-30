#include "server.h"
using namespace std;

server::server() {}
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

    //获取本地ip地址
    struct ifreq ifr;
    char ip[32] = {NULL};
    strcpy(ifr.ifr_name, "eth0");
    ioctl(this->ProxyServerFd, SIOCGIFADDR, &ifr);
    strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    std::cout << "Server IP:" << ip << " port: " << this->ProxyServerPort << endl;

    bind(this->ProxyServerFd, (struct sockaddr *)&this->ProxyServerAddr, sizeof(this->ProxyServerAddr));
    listen(this->ProxyServerFd, 8);

    //epoll init
    this->epollFd = epoll_create(100);
    struct epoll_event epev;
    epev.events = EPOLLIN;
    epev.data.fd = this->ProxyServerFd; //监听代理服务器fd
    epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->ProxyServerFd, &epev);
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

//buffer:HTTP报文整体-------unused
HttpParser server::ParseHttpHead(char *buffer)
{
    return HttpParser(buffer);
}
void server::fcntlNonBlock(int cfd)
{

    int flag = fcntl(cfd, F_GETFL);
    if (flag == -1)
    {
        perror("fcntl");
        exit(-1);
    }
    flag |= O_NONBLOCK;
    if (fcntl(cfd, F_SETFL, flag) == -1)
    {
        perror("fcntl");
        exit(-1);
    }
}

void server::Start()
{
    std::cout << "Server start..." << endl;
    std::cout << "Init socket,bind,listen,epoll_init..." << endl;
    InitSocket();
    while (1)
    {
        int ret = epoll_wait(this->epollFd, this->epollEvents, 1024, -1);
        if (ret == -1)
        {
            perror("epoll_wait");
            exit(-1);
        }
        for (int i = 0; i < ret; i++)
        {
            int clientFd = this->epollEvents[i].data.fd;
            int events = this->epollEvents[i].events;

            if (clientFd == this->ProxyServerFd)
            {
                std::cout << "New client request..." << endl;
                //有新的客户端TCP请求，加入epoll监听
                struct sockaddr_in clientAddr;
                socklen_t len = sizeof(clientAddr);

                //建立和客户端通信的的fd，即之后的clientFd
                int cfd = accept(this->ProxyServerFd, (struct sockaddr *)&clientAddr, &len);

                //设置非阻塞
                //fcntlNonBlock(cfd);

                struct epoll_event epev;
                epev.events = EPOLLIN;
                epev.data.fd = cfd;
                epoll_ctl(this->epollFd, EPOLL_CTL_ADD, cfd, &epev);
                std::cout << "Handle client request finished..." << endl;
            }
            else
            {
                //从已连接的客户端接受到信息（忽略服务器的写）
                if (events & EPOLLOUT)
                {
                    continue;
                }
                if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                {
                    closeFd(clientFd);
                }
                std::cout << "Receive msg from a client..." << endl;
                char buffer[1024] = {0};
                int len = read(clientFd, buffer, sizeof(buffer));

                if (len == -1)
                {
                    perror("read1");
                    exit(-1);
                }
                else if (len == 0) //收到了End of file
                {
                    std::cout << "A client has disconnected..." << endl; //应该在处理完这一次http请求后再断开

                    //和客户端断开连接，则取消对clientfd的监听，关闭为其打开的文件描述符
                    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, clientFd, NULL);
                    close(clientFd);
                }
                else if (len > 0)
                {
                    cout << "buffer:" << endl;
                    puts(buffer);
                    cout << "buffer size:" << len << endl;
                    std::cout << "Handle proxy logic...." << endl;

                    //读取数据并处理HTTP请求
                    this->tpl->commit(std::bind(&server::HandleHttpRequest, this, buffer, clientFd, len));
                    //HandleHttpRequest(buffer, clientFd,len);
                }
            }
        }
    }
    close(this->ProxyServerFd);
    close(this->epollFd);
}

void server::closeFd(int clientFd)
{
    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    close(clientFd);
}

/**
 * 处理http请求
 * buffer:http请求报文整体
 * clientFd:与客户端(请求发出者)通信的fd
 */
void server::HandleHttpRequest(char *buffer, int clientFd, int buffersize)
{

    //取消监听防止冲突
    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, clientFd, NULL);

    //HttpParser httpParser = HttpParser(buffer);

    //解析http头
    struct HttpHeader httpHeader;
    char *sendBuffer = new char[sizeof(buffer) + 1];
    memset(sendBuffer, 0, sizeof(sendBuffer)); //'\0'
    memcpy(sendBuffer, buffer, sizeof(buffer));
    ParseHttpHeader(buffer, &httpHeader);

    //打印解析得到的host
    std::cout << "host:";
    puts(httpHeader.host);

    //提取出host,充当客户端转发
    string host = httpHeader.host;
    //string h = "baidu.com";

    if (BanList.find(host) != BanList.end()) //BanList
    {
        std::cout << "[Ban]:" << host << endl;
        closeFd(clientFd);
        return;
    }
    else if (TransList.find(host) != TransList.end()) //TransList
    {
        std::cout << "[Trans]:" << host << endl;
        string target = TransList[host];
        const char *target_c = target.c_str();
        replace(sendBuffer, host, target);
        memcpy(httpHeader.host, target_c, target.length() + 1);
        host = target;
    }

    char *host_c = (char *)malloc(sizeof(host));
    memccpy(host_c, host.c_str(), 0, sizeof(host));

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(-1);
    }
    

    struct sockaddr_in saddr;
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = PF_INET;
    int port = 80;
    saddr.sin_port = htons(port);

    //DNS解析
    struct hostent *hostent = gethostbyname(host_c); //DNS
    if (!hostent)
    {
        std::cout << "In hostent:failed! *********" << endl;
        return;
    }
    in_addr Inaddr = *((in_addr *)*hostent->h_addr_list);

    for (int i = 0; hostent->h_addr_list[i]; i++)
    {
        cout << "IP :" << inet_ntoa(*((struct in_addr *)hostent->h_addr_list[i])) << endl;
    }

    inet_pton(AF_INET, "111.13.100.92", &saddr.sin_addr.s_addr); //inet_ntoa(Inaddr)
    //saddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr)); //connect error

    std::cout << "connecting to server "
              << "host=" << host_c << "  port=" << port << endl;

    if (connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
    {
        std::cout << "connect error..." << endl;
        std::cout << "----------------------------------" << endl;
        closeFd(clientFd);
        return;
    }

    //设置为非阻塞,一定要在connect之后设置
    //fcntlNonBlock(fd);

    std::cout << "sending http msg to server..." << endl;

    //char *request = "GET / HTTP/1.1\r\nHost: baidu.com\r\n\r\nxx"; //  Bad request

    //发送http报文
    cout << "in Write" << endl;

    int re = write(fd, buffer, buffersize); //buffer

    cout << "write size:" << re << endl;

    if (re == -1)
    {
        if (errno & EAGAIN)
        {
            //continue;
        }
        else
        {
            perror("write1");
            exit(-1);
        }
    }
    else if (re == 0)
    {
        closeFd(clientFd);
        return;
    }
    std::cout << "send finished. waiting for reponse..." << endl;

    char buf[1024];

    //获取应答
    while (1) //checked
    {
        std::cout << "Inloop" << endl;

        bzero(buf, 1024);
        int count = read(fd, buf, sizeof(buf)); //若非阻塞，判断EAGAIN
        if (count > 0)
        {
            std::cout << "receive msg from server:" << endl
                      << "-------------------------------" << endl;
            puts(buf);
            std::cout << "-------------------------------------" << endl;
            std::cout << "count:" << count << endl;
            std::cout << "writing to client..." << endl;

            //将服务器的应答转发给客户端，由于是写操作(EPOLL_OUT)，不会触发epoll

            int re = write(clientFd, buf, count); //write

            if (re == -1)
            {
                perror("write2");
                break;
            }
            else if (re > 0)
            {
                cout << "write count:" << re << endl;
            }
            else if (re == 0)
            {
                break;
            }
        }
        else if (count == -1)
        {
            if (errno & EAGAIN)
            {
                std::cout << "EAGAIN" << endl;
                break;
            }
            else
            {
                perror("read");
                break;
            }
        }
        else if (count == 0) //服务器断开连接
        {
            //读完了
            std::cout << "转发成功\n";
            break;
        }
    }
    std::cout << "proxy logic finished" << endl;
    close(fd);
    closeFd(clientFd);
}

void ParseHttpHeader(char buffer[], HttpHeader *httpHeader)
{
    char *p;
    char *ptr;
    const char *delim = "\r\n";
    p = strtok_r(buffer, delim, &ptr); //提取第一行
    if (p[0] == 'G')
    { //GET 方式
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    }
    else if (p[0] == 'P')
    { //POST 方式
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }

    p = strtok_r(NULL, delim, &ptr);
    while (p)
    {
        switch (p[0])
        {
        case 'H': //Host
            memcpy(httpHeader->host, &p[6], strlen(p) - 6);
            break;
        case 'C': //Cookie
            if (strlen(p) > 8)
            {
                char header[8];
                memset(header, 0, sizeof(header));
                memcpy(header, p, 6);
                if (!strcmp(header, "Cookie"))
                    memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);
            }
            break;
        default:
            break;
        }
        p = strtok_r(NULL, delim, &ptr);
    }
}
void replace(char buffer_c[], const string &oldstr, const string &newstr)
{
    string buffer = string(buffer_c);
    while (buffer.find(oldstr) != string::npos)
    {
        int l = buffer.find(oldstr);
        buffer = buffer.substr(0, l) + newstr + buffer.substr(l + oldstr.length());
    }
    memcpy(buffer_c, buffer.c_str(), buffer.length() + 1);
}

//Http header
