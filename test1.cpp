#include "./src/HttpHeader.h"
#include <iostream>
using namespace std;

int main()
{
    HttpHeader hh{};
    cout << sizeof(hh);
    return 0;
}