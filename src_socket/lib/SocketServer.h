#ifndef DEF_SERVER
#define DEF_SERVER

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

class SocketServer {
    private:
        int m_port;
        int m_socket;
        struct sockaddr_in m_server;

        //
        bool bUnixDomain;
        struct sockaddr_un m_unserver;

    public:
        SocketServer();
        SocketServer(int port);
        bool start();
        int accept();
};
#endif
