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

string& trim(string &s)
{
    if (s.empty())
    {
        return s;
    }
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

string launch_cmd(const char* cmd){
    FILE *fpipe;
    string result;
    char val[128] = {0};

    if (0 == (fpipe = (FILE*)popen(cmd, "r"))) {
        perror("popen() failed.");
        return "";
    }

    while (fgets(val, sizeof(val)-1,fpipe)) {
        //printf("%s", val);
        result += val;
    }
    if (result.size() > 0 && result[result.size()-1] == '\n')
        //result[result.size()-1]='\0';//tony:wrong!
        result.erase(result.end()-1);
        cout << result <<endl;
    pclose(fpipe);
    return trim(result);
}


string get_current_mac_addrs(){
    string mac = launch_cmd("ifconfig | grep HWaddr | awk '{print $NF}'");
    /*
    replace(mac.begin(), mac.end(), '\n', ';');
    if ( mac.compare("") == 0){
        mac = launch_cmd("ifconfig | grep \"硬件地址\" | awk '{print $NF}'");
        replace(mac.begin(), mac.end(), '\n', ';');
    }
    if ( mac.compare("") == 0){
        cout<<"Failed to get mac addr"<<endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    */
    return mac;
}



static const char ENV_DEBUG[]="DEBUG";
bool isDebugEnv(){
    const char *debug = getenv(ENV_DEBUG);
    if (debug == nullptr){
        cout<<"ENV_DEBUG is not set, assume false"<<endl;
        return false;
    }
    if (strcmp(debug, "true") == 0){
        return true;
    }
    return false;
}

#include <stdio.h>
#include <string.h>
#include <shadow.h>
#include <unistd.h>

static const char user_passwd[]="invalid_passwd";
int check_passwd(const char* name = NULL){
    const char *username = NULL;
    if (name == NULL || strcmp(name, "") == 0)
        username = getlogin();
    else
        username = name;

    cout<<"current username: " << username<<endl;
    struct spwd  *sp;
    sp = getspnam(username);
    if(sp == NULL) {
        printf("get spentry error. %s\n", strerror(errno));
        return -1;
    }

    if(strcmp(sp->sp_pwdp, (char*)crypt(user_passwd, sp->sp_pwdp)) == 0) {
        printf("passwd check success\n");
        return 0;
    }
    else {
        printf("passwd check fail\n");
        return -1;
    }
}


const int stdoutfd(dup(fileno(stdout)));
const int stderrfd(dup(fileno(stdout)));
int redirect_stdout_stderr(const char* fname){
    int newdest = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    fflush(stdout);
    fflush(stderr);
    dup2(newdest, fileno(stdout));
    dup2(newdest, fileno(stderr));
    close(newdest);
    return 0;
}

int restore_stdout(){
    fflush(stdout);
    dup2(stdoutfd, fileno(stdout));
    dup2(stderrfd, fileno(stderr));
    //comment if it need reused
    //close(stdoutfd);
    //close(stderrfd);
    return 0;
}
