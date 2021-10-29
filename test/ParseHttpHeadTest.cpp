#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>
#include <vector>
#include <regex>
#include "HttpHeader.h"
#include <string.h>
#include <stdlib.h>
using namespace std;

HttpHeader a(char *buffer)
{
    return HttpHeader(buffer);
}

int main()
{

    char *str = "GET xx 1.1\r\nk1:12\r\nk2:23\r\nk3:34\r\n\r\nxx\r\nxx"; //  [line 2]
    puts("aaaa\n");
    puts(str);
    //HttpHeader hh = HttpHeader(str);
    HttpHeader hh = HttpHeader(str);
    //a(str, &hh);
    hh.print();
    puts("aaaa\n");
    char *line = "newline";
    char *des = (char *)malloc(sizeof(str) + sizeof(line));
    HttpHeader::CatLine(str, des, line);
    puts("des:\n");
    puts(des);

    hh.Parse(des);
    hh.print();

    //hh.print();
    exit(0);
    //
    //    char *p;
    //    char *ptr;
    //    char data[] = "GET xx\r\nPOST xx \r\nxx\r\n\r\n";
    //    char delim[] = "\r\n";
    //
    //    //string s = "a";
    //    //char *c = (char *)malloc(sizeof(s));
    //    //memccpy(c, s.c_str(), 0, sizeof(s));
    //    //puts(c);
    //    //cout << strlen(c) << endl;
    //    //exit(0);
    //
    //    //string s = c;
    //
    //    //vector<int>::iterator it = search(0, strlen(data) - 1, 0, strlen(delim) - 1);
    //
    //    std::string strPattern("([\\w\\s:/\\ &\\.]*)\r\n\r\n.*"); // [line 1]
    //    std::regex r(strPattern);
    //    std::smatch result;
    //    std::string str_test = "GET xx 1.1\r\nk1:12\r\nk2:23\r\nk3:34\r\n\r\nxx\r\nxx"; //  [line 2]
    //
    //    char *header;
    //    if (std::regex_search(str_test, result, r))
    //    {
    //        //std::cout << result.str() << std::endl;
    //        //int size = result.size();
    //        //cout << size << endl;
    //        //int i = 0;
    //        //while (i < size)
    //        //{
    //        //    cout << result.str(i) << "Index:" << result.position(i) << endl;
    //        //    i++;
    //        //}
    //        header = (char *)malloc(sizeof(result.str(1)));
    //        memccpy(header, result.str(1).c_str(), 0, sizeof(result.str(1)));
    //        puts(header);
    //        puts("\n");
    //    }
    //    else
    //    {
    //        std::cout << "smatch failed !" << std::endl;
    //    }
    //
    //    int maxline = 20;
    //    char *line[maxline]; //最多20行
    //    char *block;
    //    char delim_line[] = "\r\n";
    //    char delim_block[] = " ";
    //    char delim_keyvalue[] = ":";
    //
    //    char *key[maxline - 1];
    //    char *value[maxline - 1];
    //    char *method, *url, *version;
    //
    //    line[0] = strtok(header, delim_line);
    //    int i = 0;
    //    while (i < maxline && line[i])
    //    {
    //        puts(line[i]);
    //        line[++i] = strtok(NULL, delim_line);
    //    }
    //    int lineCount = i;
    //    //parse first line
    //    method = strtok(line[0], delim_block);
    //    url = strtok(NULL, delim_block);
    //    version = strtok(NULL, delim_block);
    //
    //    cout << "method:" << method << endl;
    //    cout << "url:" << url << endl;
    //    cout << "version:" << version << endl;
    //
    //    i = 1;
    //    int index = 0;
    //    while (i < lineCount)
    //    {
    //        key[index] = strtok(line[i], delim_keyvalue);
    //        value[index] = strtok(NULL, delim_keyvalue);
    //        cout << "key=" << key[index] << "  value=" << value[index] << endl;
    //        index++;
    //        i++;
    //    }

    //    cout << "method:" << hh.method << endl;
    //    cout << "url:" << hh.url << endl;
    //    cout << "version:" << hh.version << endl;
    //
    //    i = 1;
    //    index = 0;
    //    while (i < hh.lineCount)
    //    {
    //        cout << "key=" << hh.key[index] << "  value=" << hh.value[index] << endl;
    //        index++;
    //        i++;
    //    }
}