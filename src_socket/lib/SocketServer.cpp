#include "SocketServer.h"
#include <stdio.h>
#include <unistd.h>
#include "stddef.h"

SocketServer::SocketServer(int port){
    m_port = port;

    m_server.sin_family = AF_INET;
    m_server.sin_addr.s_addr = INADDR_ANY;
    m_server.sin_port = htons(port);
    bUnixDomain = false;
}

//ylteng: Unix domain socket
SocketServer::SocketServer(){
    unlink("ipc_socket");               /* in case it already exists */
    memset(&m_unserver, 0, sizeof(m_unserver));
    m_unserver.sun_family = AF_UNIX;
    strcpy(m_unserver.sun_path, "ipc_socket");

    bUnixDomain = true;
}

bool SocketServer::start(){
    if (bUnixDomain){
        m_socket = ::socket(AF_UNIX , SOCK_STREAM , 0);

        if(m_socket!=-1){
            //if(::bind(m_socket,(struct sockaddr *)&m_unserver , sizeof(m_unserver)) >= 0){
            int len = offsetof(struct sockaddr_un, sun_path) + strlen("ipc_socket");
            if(::bind(m_socket,(struct sockaddr *)&m_unserver , len) >= 0){
                ::listen(m_socket, 3);
                return true;
            }
            else
                perror("fail to bind socket");
        }
        else
            perror("fail to create socket");
    }
    else{
        m_socket = ::socket(AF_INET , SOCK_STREAM , 0);

        if(m_socket!=-1){
            if(::bind(m_socket,(struct sockaddr *)&m_server , sizeof(m_server)) >= 0){
                ::listen(m_socket, 3);
                return true;
            }
            else
                perror("fail to bind socket");
        }
        else
            perror("fail to create socket");
    }
    return false;
}

int SocketServer::accept(){
    int c = sizeof(struct sockaddr_in);

    if (bUnixDomain){
        struct sockaddr_un client;

        int client_sock = ::accept(m_socket, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0){
            perror("accept error");
            return -1;
        }
        return client_sock;
    }
    else{
        struct sockaddr_in client;

        int client_sock = ::accept(m_socket, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0){
            perror("accept error");
            return -1;
        }
        return client_sock;
    }
}
