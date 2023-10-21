#include <iostream>
#include "XLex.h"
using namespace std;
int main()
{
    string input;
    XLex e;
    while (cin >> input)
    {
        if (!e.Read(input))
            cout << "输入了不支持的字符！" << endl;
        else
        {
            e.toSuffix();
            e.toNFA();
            e.ShowNFA();
            e.toDFA();
            cout << endl;
            e.ShowDFA();
            e.Reset();
        }
    }
}

