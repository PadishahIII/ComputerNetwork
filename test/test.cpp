#include "threadpool.h"
#include <iostream>
using namespace std;
class a
{
public:
    static int p1(int i)
    {
        cout << i << endl;
        return 0;
    }
};

int main()
{
    //    threadpool tpl{4};
    //
    //    a A;
    //    future<int> ff = tpl.commit(A.p1, 2);
    //    //cout << "return:" << ff.get();
    //    getchar();
    char c[2];
    cout << sizeof(c);
    return 0;
}
