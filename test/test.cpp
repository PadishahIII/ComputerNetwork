#include "threadpool.h"
#include <iostream>
using namespace std;
class test
{
public:
    int p1(int i)
    {
        cout << i << endl;
        return 0;
    }
    threadpool *tpl;
    void Start()
    {
        tpl->commit(std::bind(&test::p1, this, 45));
    }
};

int main()
{
    threadpool tpl{4};
    //
    //    a A;
    //    future<int> ff = tpl.commit(A.p1, 2);
    //    //cout << "return:" << ff.get();
    //    getchar();
    test t = test();
    t.tpl = &tpl;
    t.Start();
    return 0;
}
