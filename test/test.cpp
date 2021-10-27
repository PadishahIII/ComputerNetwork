#include "threadpool.h"
#include <iostream>
using namespace std;

int p1(int i)
{
    cout << i << endl;
    return 0;
}

int main()
{
    threadpool tpl{4};

    future<int> ff = tpl.commit(p1, 2);
    //cout << "return:" << ff.get();
    getchar();
    return 0;
}
