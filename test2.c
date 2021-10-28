#include <stdio.h>
#include <string.h>

int main()
{
    char *p;
    char *ptr;
    char data[] = "GET xx\rPOST xx \r\n";
    char delim[] = "\r\n";

    p = find(0, strlen(data), "\r\n");
    puts(p);

    //p = strtok(data, delim);
    //while (p)
    //{
    //    puts(p);
    //    p = strtok(NULL, delim);
    //}
}