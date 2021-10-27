#ifndef HTTPHEADER_H
#define HTTPHEADER_H

#include <stdlib.h>
#include <strings.h>

struct HttpHeader
{
    char method[4]; //POST  GET  CONNECT
    char url[1024];
    char host[1024]; //target host
    char cookie[1024 * 10];
    HttpHeader()
    {
        bzero(this, sizeof(HttpHeader));
    }
};

#endif //HTTPHEADER_H