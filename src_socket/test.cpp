
#include <string>
#include <iostream>

using namespace std;
int main()
{
    int aa = 2;
    string a="\"a:\"" + to_string(aa);
    cout << a << endl;
    return 0;
}
