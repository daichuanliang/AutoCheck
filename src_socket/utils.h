#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <thread>
#include <chrono>
#include <iostream>
//#include <boost/ref.hpp>
//#include <boost/asio.hpp>
//#include <boost/bind.hpp>
//#include <boost/function.hpp>

using namespace std;

string& trim(string &s);

string launch_cmd(const char* cmd);
string get_current_mac_addrs();

bool isDebugEnv();

int check_passwd(const char* name = NULL);

int redirect_stdout_stderr(const char* fname);
int restore_stdout();
