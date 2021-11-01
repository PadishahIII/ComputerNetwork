#include "server.h"
using namespace std;

server::server()
{
}
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
    listen(this->ProxyServerFd, 10);

    //epoll init
    epoller->AddFd(this->ProxyServerFd, EPOLLIN);
}

/**
 * 连接服务器，返回与之通信的socket(fd)
 * -1:error
 */
int server::ConnectToServer(string host, int port)
{

    char *host_c = (char *)malloc((host.size() + 1));
    memccpy(host_c, host.c_str(), 0, (host.size() + 1));

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in saddr;
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = PF_INET;
    int port_ = port; //TODO:
    saddr.sin_port = htons(port_);

    //DNS解析
    struct hostent *hostent = gethostbyname(host_c); //DNS
    if (!hostent)
    {
#ifdef debug
        std::cout << "In hostent:failed! *********" << endl;
#endif
        close(fd);
        return -1;
    }
    in_addr Inaddr = *((in_addr *)*hostent->h_addr_list);
#ifdef printIP
    for (int i = 0; hostent->h_addr_list[i]; i++)
    {
        cout << "IP :" << inet_ntoa(*((struct in_addr *)hostent->h_addr_list[i])) << endl;
    }
#endif

    //inet_pton(AF_INET, "111.13.100.92", &saddr.sin_addr.s_addr); //inet_ntoa(Inaddr)
    saddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr)); //connect error

    if (connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
    {
        std::cout << "connect error..." << endl;
        std::cout << "----------------------------------" << endl;
        close(fd);
        return -1;
    }
#ifdef debug
    std::cout << "already connect to server "
              << "host=" << host_c << "  port=" << port << endl;
#endif
    free(host_c);

    return fd;
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
        int eventCnt = epoller->Wait(-1); //无事件将阻塞
        for (int i = 0; i < eventCnt; i++)
        {
            int fd = epoller->GetEventFd(i);
            uint32_t events = epoller->GetEvents(i);
            if (fd == this->ProxyServerFd)
            {
                DealListen();
            }
            else if (events & EPOLLOUT)
            {
                //从已连接的客户端接受到信息（忽略服务器的写）
                continue;
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                CloseConn(fd);
            }
            else if (events & EPOLLIN)
            {
                char *buffer = new char[this->MAXSIZE];
                bzero(buffer, this->MAXSIZE);
                int len = DealRead(buffer, fd);
                this->tpl->commit(std::bind(&server::HandleHttpRequest, this, buffer, fd, len)); //TODO:
                //HandleHttpRequest(fd, len);
            }
        }
        //sleep(1);
    }
}
void server::DealListen()
{
#ifdef debug
    std::cout << "New client request..." << endl;
#endif
    //有新的客户端TCP请求，加入epoll监听
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    //建立和客户端通信的的fd，即之后的clientFd
    int cfd = accept(this->ProxyServerFd, (struct sockaddr *)&clientAddr, &len);
    if (cfd <= 0)
        return;
#ifdef IP
    std::cout << "[IP]:" << inet_ntoa(clientAddr.sin_addr) << endl;
#endif
#ifdef debug
    std::cout << "fd:" << cfd << endl;
#endif
    epoller->AddFd(cfd, EPOLLIN);
#ifdef debug
    std::cout << "Handle client request finished..." << endl;
#endif
}

int server::DealRead(char *buffer, int fd)
{
#ifdef debug
    std::cout << "Receive msg from a client..." << endl;
#endif
    //char buffer[this->MAXSIZE] = {0};
    int ret = Recv(fd, buffer, MAXSIZE, 0);
    if (ret <= 0)
    {
        //和客户端断开连接，则取消对clientfd的监听，关闭为其打开的文件描述符
        CloseConn(fd);
        return ret;
    }
#ifdef debug
    cout << "buffer:" << endl;
    puts(buffer);
    cout << "buffer size:" << ret << endl;
    std::cout << "Handle proxy logic...." << endl;
#endif
    //buf = buffer;
    return ret;
    //读取数据并处理HTTP请求

    //this->tpl->commit(std::bind(&server::HandleHttpRequest, this, buffer, fd, ret));
    //HandleHttpRequest(buffer, clientFd, len);
}
void server::CloseConn(int clientFd)
{
    epoller->DelFd(clientFd);
    close(clientFd);
}

/**
 * 处理http请求
 * buffer:http请求报文整体
 * clientFd:与客户端(请求发出者)通信的fd
 */
void server::HandleHttpRequest(char *buffer, int clientFd, int buffersize)
{
    //解析http头的过程会改变buffer
    //char *copyBuffer = new char[buffersize];
    //memcpy(copyBuffer, buffer, buffersize);

    //解析http头
    struct HttpHeader httpHeader;
    char sendBuffer[buffersize + 20];
    memset(sendBuffer, 0, sizeof(sendBuffer)); //'\0'
    memcpy(sendBuffer, buffer, buffersize);
    if (!ParseHttpHeader(buffer, &httpHeader, sendBuffer))
    {
        SendError(clientFd, "[Banned Host]");
        CloseConn(clientFd);
        delete buffer;
        //sleep(1);
        return;
    }

    string host = httpHeader.host;
    string method = httpHeader.method;
#ifdef debug
    std::cout << "method:" << method << "  compare:" << method.compare("CONNECT") << endl;
#endif

    //不转发connect报文（因为这是发给代理服务器的）
    if (method.compare("GET") != 0 && method.compare("POST") != 0)
    {
#ifdef debug
        std::cout << "encounter a CONNECT msg" << std::endl;
#endif
        SendError(clientFd, "HTTP/1.0 200 Connection Established\r\nProxy-agent: Netscape-Proxy/1.1\r\n\r\n");
        CloseConn(clientFd);
        delete buffer;
        //sleep(1);
        return;
    }
    int re;
    int fd = ConnectToServer(host, httpHeader.port);
#ifdef run
    std::cout << "[Host]:" << host << " [Port]:" << httpHeader.port << std::endl;
#endif
    if (fd == -1)
    {
        SendError(clientFd, "Failed to Connect to server\n");
        goto error;
    }

#ifdef debug
    std::cout << "sending http msg to server..." << endl;
#endif

    //发送http报文
    re = Send(fd, sendBuffer, strlen(sendBuffer) + 1, 0);
    if (re <= 0)
        goto error;

#ifdef debug
    std::cout << "send finished. waiting for reponse..." << endl;
#endif

    //获取应答
    while (1) //recv
    {
        int count = Recv(fd, buffer, MAXSIZE, 0);
        if (count > 0)
        {
#ifdef debug
            //puts(sendBuffer);
            std::cout << "count:" << count << endl;
            std::cout << "writing to client..." << endl;
#endif
            int re = Send(clientFd, buffer, count, 0);
            if (re <= 0)
                goto error;
#ifdef debug
            std::cout << "receive msg from server:" << endl
                      << "-------------------------------" << endl;
#endif
#ifdef debug
            std::cout << "-------------------------------------" << endl;
#endif
        }
        else
        {
            SendError(clientFd, "Failure");
            goto error;
        }
    }
#ifdef debug
    std::cout << "proxy logic finished" << endl;
#endif
error:
    CloseConn(clientFd);
    close(fd);
    delete buffer;
    //sleep(1);
    return;
}

bool server::ParseHttpHeader(char buffer[], HttpHeader *httpHeader, char *sendBuffer)
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
        {
            if (p[1] != 'o' && p[1] != 'O')
                break;
            char *ptr_port;
            char *p_without_port;
            p_without_port = strtok_r((p + 6), ":", &ptr_port);
            char *p_port = strtok_r(NULL, ":", &ptr_port);
            int port = 80;
            if (p_port)
                port = atoi(p_port);
            if (port > 0)
            {
                //设置port
                httpHeader->port = port; //
            }
            memcpy(httpHeader->host, p_without_port, strlen(p_without_port)); //&p[6]
            //memcpy(httpHeader->host, &p[6], strlen(p) - 6); //&p[6]
            break;
        }
        case 'C': //Cookie
            if (p[1] != 'o' && p[1] != 'O')
                break;
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
#ifdef debug
    //打印解析得到的host
    std::cout << "host:";
    puts(httpHeader->host);
    std::cout << "port:";
    std::cout << to_string(httpHeader->port) << std::endl;

#endif
    string host = httpHeader->host;

    if (BanList.find(host) != BanList.end()) //BanList
    {
#ifdef run
        if (host.c_str()[0] != 0) //TODO:匹配到""则不打印
            std::cout << "[Ban]:" << host << endl;
#endif
        //CloseConn(clientFd);
        return false;
    }
    else if (TransList.find(host) != TransList.end()) //TransList
    {
        string target = TransList[host];
#ifdef run
        std::cout << "[Trans]:" << host << " TO " << target << std::endl;
#endif
        const char *target_c = target.c_str();
        replace(sendBuffer, host, target);
        memcpy(httpHeader->host, target_c, target.length() + 1);
        host = target;
    }
    return true;
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
int server::Recv(int fd, void *buffer, size_t size, int flags)
{
    int len = recv(fd, buffer, size, flags);

    if (len == -1)
    {
        perror("recv");
#ifdef debug
        std::cout << "error fd is:" << fd << endl;
#endif
    }
    else if (len == 0) //收到了End of file
    {
#ifdef debug
        std::cout << "A client has disconnected,fd=" << fd << endl; //应该在处理完这一次http请求后再断开
#endif
    }
    return len;
}
int server::Send(int fd, const void *buffer, size_t len, int flags)
{
    int re = send(fd, buffer, len, flags);
#ifdef debug
    cout << "write size:" << re << endl;
#endif
    if (re == -1)
    {
#ifdef debug
        perror("send");
#endif
    }
    else if (re == 0)
    {
    }
    return re;
}
int server::Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *tv)
{
    int count = select(0, readfds, NULL, NULL, tv); //0代表不用内核检测，超时检测TODO:read=>select

    if (count < 0) //recv
    {
#ifdef debug
        perror("select");
#endif
    }
    else if (count == 0) //服务器断开连接
    {
    }
    return count;
}

void server::SendError(int fd, char *buf)
{
    int ret = send(fd, buf, strlen(buf), 0);
    if (ret < 0)
    {
#ifdef debug
        perror("senderror");
        std::cout << "send error fd:" << fd << std::endl;
#endif
    }
}