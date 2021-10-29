#include "src/server.h"
#include <iostream>
using namespace std;

//./bin/server 8889
int main(int argc, char **args)
{
    cout << "in main" << endl;
    threadpool tpl{10};

    server s = server(atoi(args[1]));
    s.tpl = &tpl;
    s.Start();
    return 0;
}