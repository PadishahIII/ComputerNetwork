#include "../src/threadpool.h"
#include <iostream>

class server
{
public:
    threadpool *tpl;
    server() {}
    void Start();
}