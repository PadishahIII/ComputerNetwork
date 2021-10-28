#include "./src/HttpHeader.h"
#include <iostream>
#include <string.h>
using namespace std;

int main()
{
    char *p;
    char *ptr;
    char *data = "GET xx\r\nPOST xx \r\n";
    char *delim = "\r\n";

    p = strtok(data, delim);
    cout << p << endl;

    //cout << strtok(NULL, delim);
    //cout << strtok(NULL, delim);

    return 0;
}