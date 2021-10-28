#ifndef HTTPHEADER_H
#define HTTPHEADER_H

#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <string.h>
#include <regex>
using namespace std;

class HttpHeader
{
public:
    char *method; //POST  GET  CONNECT
    char *url;
    char *version;
    char *host = NULL; //target host
    char *cookie = NULL;

    static const int maxline = 20; //http头最大行数

    char *line[maxline];
    char *key[maxline - 1];
    char *value[maxline - 1];

    int lineCount; //报头的实际行数

    HttpHeader::HttpHeader() {}
    //buffer:HTTP整体报文
    HttpHeader::HttpHeader(char *buffer)
    {
        bzero(this, sizeof(HttpHeader));
        char *header;
        std::string HeaderPattern("([\\w\\s:/\\ &\\.]*)\r\n\r\n.*");
        //std::string LinePattern("([\\w\\s:/\\ &]*)\r\n([\\w\\s:/\\ &]*)\r\n([\\w\\s:/\\ &]*)\r\n([\\w\\s:/\\ &]*)\r\n.*");
        std::regex r(HeaderPattern);
        std::smatch result;
        std::string str = buffer; //  [line 2]

        if (std::regex_search(str, result, r)) //将http头提取出来
        {
            header = (char *)malloc(sizeof(result.str(1)));
            memccpy(header, result.str(1).c_str(), 0, sizeof(result.str(1)));
            //puts("Capture HTTP header:\n");
            //puts(header);
        }
        else
        {
            std::cout << "Invaild HTTP header !" << std::endl;
        }

        char delim_line[] = "\r\n";
        char delim_block[] = " ";
        char delim_keyvalue[] = ":";

        //分离出每一行
        line[0] = strtok(header, delim_line);
        int i = 0;
        while (i < maxline && line[i])
        {
            //puts(line[i]);
            line[++i] = strtok(NULL, delim_line);
        }
        this->lineCount = i; //http头中总的行数

        //parse first line
        this->method = strtok(line[0], delim_block);
        this->url = strtok(NULL, delim_block);
        this->version = strtok(NULL, delim_block);

        //cout << "method:" << method << endl;
        //cout << "url:" << url << endl;
        //cout << "version:" << version << endl;

        //parse key:value
        i = 1;
        int index = 0;
        while (i < lineCount)
        {
            key[index] = strtok(line[i], delim_keyvalue);
            value[index] = strtok(NULL, delim_keyvalue);
            //cout << "key=" << key[index] << "  value=" << value[index] << endl;
            index++;
            i++;
        }

        //Build host and cookie
        i = 0;
        while (i < this->lineCount - 1)
        {
            if (key[i][0] == 'H')
            {
                host = value[i];
            }
            else if (key[i][0] == 'C')
            {
                cookie = value[i];
            }
            i++;
        }
    }
    void HttpHeader::print()
    {
        //cout << "WHOLE HTTP HEADER:\n";
        //for (int i = 0; i < this->lineCount; i++)
        //    puts(line[i]);
        cout << "COUNT OF LINE:" << this->lineCount << endl;
        cout << "METHOD:" << method << endl;
        cout << "URL:" << url << endl;
        cout << "VERSION:" << version << endl;
        cout << "KEY&VALUE:" << endl;
        for (int i = 0; i < this->lineCount - 1; i++)
        {
            cout << "KEY:" << this->key[i] << " VALUE:" << this->value[i] << endl;
        }
        if (!this->host)
            cout << "No Host Domain" << endl;
        else
            cout << "Host:" << this->host << endl;
        if (!this->cookie)
            cout << "No Cookie Domain" << endl;
        else
            cout << "Cookie:" << this->cookie << endl;
    }
};

#endif //HTTPHEADER_H