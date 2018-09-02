#ifndef DEF_SOCKETCLIENT
#define DEF_SOCKETCLIENT

#include <string>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>
#include <sys/un.h>

#include "Base64.h"
#include "common/message.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

class SocketClient {
    private:
        void *m_tag;
        //unix domain socket
        struct sockaddr_un m_unserver;
        bool bUnixDomain;
	    float frmrate;
        char clientName[64];

        struct sockaddr_in m_server;

        std::string sMac;
        std::string m_hostname;
        std::string m_address;
        int m_port;
        int m_socket;
        bool m_connected;
        bool m_threadStopped;

        int m_packetSize;

        pthread_t m_thread;
        std::map<std::string, void (*)(SocketClient*, std::string)> m_messageListenerMap;
        void (*m_disconnectListener) (SocketClient*);

        void receiveThread();
        static void* staticReceiveThread(void* p){
            SocketClient *client = (SocketClient*) p;
            client->receiveThread();
        }
        int receive(std::string &message);
        bool send(std::string message);

        int receive_buf(void* buf, uint32_t length);

    public:
        SocketClient();
        SocketClient(std::string address, int port);
        SocketClient(int socket, const char* name = "");

        int getSocket();
        void* getTag();

        bool connect();
        void disconnect();
        bool send(std::string key, std::vector<std::string> messages);
        bool send_simple(std::string key, std::string message);
        void addListener(std::string key, void (*messageListener) (SocketClient*, std::string));
        void setDisconnectListener(void (*disconnectListener) (SocketClient*));
        void setTag(void *tag);

        void setMac(const std::string mac){
            sMac = mac;
        }
        std::string getMac(){ return sMac;}
};

#endif
