#include <iostream>
#include <thread>
#include "common/message.h"
#include "lib/SocketClient.h"
#include "utils.h"

using namespace std;

void onMessage(SocketClient *sender, string message){
    cout<<"message: "<<message<<endl;
}

void onMessage_advertise(SocketClient *sender, string message){
    cout<<"message: "<<message<<endl;
}

void onDisconnect(SocketClient *socket){
    cout << "you have been disconnected" << endl;
}

#ifdef DebugMain
int main(int argc , char *argv[])
{
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

#ifdef USE_UNIX_DOMAIN
    SocketClient client;
    cout<<"Client try connect to UNIX domain socket"<<endl;;
#else
    SocketClient client(SOCKET_IP_ADDR, SOCKET_PORT);
    cout<<"Client try connect to "<<SOCKET_IP_ADDR<<":"<<SOCKET_PORT<<endl;
#endif
    client.setDisconnectListener(onDisconnect);

    client.addListener("message", onMessage);
    client.addListener("advertise", onMessage_advertise);

    if(!client.connect()){
        perror("connect error");
        cout << "could not connect to server" << endl;
        return 0;
    }
    cout << "connected to server." << endl;

    //register cilent mac
    string&& macs=get_current_mac_addrs();
    cout<<"this client's macs: " << macs<<endl;
    if (!client.send_simple("register", macs)){
        perror("register error");
        cout<<"fail to register"<<endl;
        return 0;
    }

    std::string line;
    while(1){
        cout << "input: ";
        getline(cin, line);
        if(!client.send_simple("message", line)){
            perror("message error");
            cout << "failed to send message" << endl;
            return 0;
        }
    }

    client.disconnect();

    return 0;
}

#endif
