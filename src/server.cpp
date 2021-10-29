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
    cout << "Server IP:" << ip << " port: " << this->ProxyServerPort << endl;

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

//buffer:HTTP报文整体
HttpParser server::ParseHttpHead(char *buffer)
{
    return HttpParser(buffer);
}

void server::Start()
{
    cout << "Server start..." << endl;
    cout << "Init socket,bind,listen,epoll_init..." << endl;
    InitSocket();
    while (1)
    {
        //cout << "Wait log in..." << endl;
        int ret = epoll_wait(this->epollFd, this->epollEvents, 1024, -1);
        if (ret == -1)
        {
            perror("epoll_wait");
            exit(-1);
        }
        for (int i = 0; i < ret; i++)
        {
            int clientFd = this->epollEvents[i].data.fd;
            if (clientFd == this->ProxyServerFd)
            {
                //cout << "New client request..." << endl;
                //有新的客户端TCP请求，加入epoll监听
                struct sockaddr_in clientAddr;
                socklen_t len = sizeof(clientAddr);

                //建立和客户端通信的的fd,设置为非阻塞
                int cfd = accept(this->ProxyServerFd, (struct sockaddr *)&clientAddr, &len);
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
                struct epoll_event epev;
                epev.events = EPOLLIN;
                epev.data.fd = cfd;
                epoll_ctl(this->epollFd, EPOLL_CTL_ADD, cfd, &epev);
                //cout << "Handle client request finished..." << endl;
            }
            else
            {
                //从已连接的客户端接受到信息（忽略服务器的写）
                if (this->epollEvents[i].events & EPOLLOUT)
                {

                    continue;
                }
                //cout << "Receive msg from a client..." << endl;
                char buffer[65536] = {0};
                int len = read(clientFd, buffer, sizeof(buffer));
                //cout << "Size=" << len << "======" << endl;
                if (len == -1)
                {
                    perror("read1");
                    exit(-1);
                }
                else if (len == 0) //收到了End of file
                {
                    //cout << "A client has disconnected..." << endl; //应该在处理完这一次http请求后再断开
                    //和客户端断开连接，则取消对clientfd的监听，关闭为其打开的文件描述符
                    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, clientFd, NULL);
                    close(clientFd);
                }
                else if (len > 0)
                {
                    //int re = len;
                    //int sum = sizeof(buffer) - len;
                    //cout << "Reading..." << endl;
                    //while (re > 0)
                    //{
                    re = read(clientFd, buffer, sizeof(buffer));
                    //sum -= re;
                    //}
                    //cout << "Handle proxy logic...." << endl;

                    //读取数据并处理HTTP请求
                    this->tpl->commit(std::bind(&server::HandleHttpRequest, this, buffer, clientFd));
                    //HandleHttpRequest(buffer, clientFd);
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
 * 
 * 
 */
void server::HandleHttpRequest(char *buffer, int clientFd)
{
    //解析http头
    //HttpParser httpParser = HttpParser(buffer);
    struct HttpHeader hh;
    char sendBuffer[1024];
    ParseHttpHeader(buffer, &hh, sendBuffer);

    //设置为非阻塞
    int flag = fcntl(clientFd, F_GETFL);
    flag | O_NONBLOCK;
    if (fcntl(clientFd, F_SETFL, flag) == -1)
    {
        perror("fcntl");
        exit(-1);
    }

    //cout << "Buffer:*" << endl;
    //puts(buffer);
    //cout << "HTTP request:" << endl;
    //cout << endl
    //     << "-----------------------------" << endl;
    ////cout << "method:" << HttpParser.method << endl;
    //httpParser.show();
    //cout
    //    << "-------------------------------" << endl
    //    << endl;

    cout << "host:";
    //cout << httpParser["host"] << endl;
    puts(hh.host);

    //提取出host,充当客户端转发
    //string h = httpParser["host"];
    string h = hh.host;

    if (BanList.find(h) != BanList.end()) //TODO:BanList
    {
        //cout << BanList.end() << endl;
        cout << "[Ban]:" << h << endl;
        closeFd(clientFd);
        return;
    }
    //    if (PassList.find(h) != PassList.end())
    //        ;
    //    else
    //    {
    //        //cout << PassList.end() << endl;
    //        //cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@:" << h.compare("error.baidu.com") << endl;
    //
    //        //cout << "[CannotPass]:" << h << endl;
    //        //closeFd(clientFd);
    //        //return;
    //    }
    char *host = (char *)malloc(sizeof(h));
    memccpy(host, h.c_str(), 0, sizeof(h));

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(-1);
    }
    struct sockaddr_in saddr;
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = PF_INET;
    int port = this->ProxyServerPort;
    saddr.sin_port = htons(port);

    //TODO:
    struct hostent *hostent = gethostbyname(host); //DNS
    if (!hostent)
    {
        cout << "In hostent:failed! *********" << endl;
    }
    in_addr Inaddr = *((in_addr *)*hostent->h_addr_list);
    //TODO:

    //inet_pton(AF_INET, "111.13.100.92", &saddr.sin_addr.s_addr); //TODO:
    saddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));

    cout << "connecting to server "
         << "host=" << host << "  port=" << port << endl;
    //return; //TODO
    if (connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
    {
        //perror("connect");
        //exit(-1);
        cout << "connect error..." << endl;
        cout << "----------------------------------" << endl;
        return;
    }
    cout << "sending http msg to server..." << endl;

    char buf[1024];

    //char *b = "GET / HTTP/1.1\r\nHost: baidu.com\r\n\r\n";
    //发送http报文
    if (write(fd, buffer, sizeof(buffer)) == -1)
    {
        perror("write");
        exit(-1);
    }
    //puts(buffer);
    cout << "send finished. waiting for reponse..." << endl;
    //获取应答
    while (1) //checked
    {

        bzero(buf, 1024);
        int count = read(fd, buf, sizeof(buf));
        if (count > 0)
        {
            cout << "receive msg from server:" << endl
                 << "-------------------------------" << endl;
            puts(buf);
            cout << "-------------------------------------" << endl;
            cout << "count:" << count << endl;
            cout << "writing to client..." << endl;

            if (count < 1024)
            {
                //读完了
                cout << "转发成功\n";
                return;
            }

            //将服务器的应答转发给客户端，由于是写操作(EPOLL_OUT)，不会触发epoll
            int re = write(clientFd, buf, sizeof(buf)); //TODO:strlen(buffer)+1
            if (re == -1)
            {
                perror("write");
                //exit(-1);
                return;
            }
        }
        else if (count == -1)
        {
            perror("recv");
            //exit(-1);
            return;
        }
        else if (count == 0)
        {
            //读完了
            cout << "转发成功\n";
            return;
        }
    }
    closeFd(clientFd);
}

void ParseHttpHeader(char buffer[], HttpHeader *httpHeader, char sendBuffer[])
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

#ifdef test
    printf("recv client = [%s]\n", p);
#endif
    p = strtok_r(NULL, delim, &ptr);
    while (p)
    {
#ifdef test
        printf("recv client = [%s]\n", p);
#endif
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

//Http header
