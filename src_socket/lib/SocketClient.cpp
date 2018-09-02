#include "SocketClient.h"
#include <string.h>
#include <iostream>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
using namespace std;


SocketClient::SocketClient(std::string address, int port){

    const string NUMSTR="0123456789";
    if (address.size()>0 && NUMSTR.find(address[0]) == string::npos){
        cout<<"address not start with numeric, conver to ip"<<endl;

        extern int h_errno;
        struct hostent *h;
        h=gethostbyname(address.c_str());
        if(h==NULL) {
            printf("%s\n",hstrerror(h_errno));
        }
        else {
            struct sockaddr_in addr_in;
            memcpy(&m_server.sin_addr.s_addr,h->h_addr,4);

            struct in_addr in;
            in.s_addr=m_server.sin_addr.s_addr;
            printf("host name:%s\n",h->h_name);
            printf("ip lenght:%d\n",h->h_length);//IPv4 or IPv6
            printf("type:%d\n",h->h_addrtype);
            printf("ip:%s\n",inet_ntoa(in));//½«һ¸öª»»³É»¸öªÍ±êµã¸ñÄַû

            m_address = inet_ntoa(in);
            m_hostname = address;
        }
    }
    else{
        m_port = port;
        m_server.sin_addr.s_addr = inet_addr(address.c_str());

        m_address = address;
        m_hostname = "";
    }


    m_server.sin_family = AF_INET;
    m_server.sin_port = htons(port);

    m_tag = NULL;
    m_disconnectListener = NULL;
    m_connected = false;
    m_threadStopped = false;
    m_packetSize = 4096;

    bUnixDomain = false;
    strcpy(clientName, "");
}

//ylteng: UNIX domain socket
SocketClient::SocketClient(){
    m_unserver.sun_family = AF_UNIX;
    strcpy(m_unserver.sun_path, UNIX_DOMAIN_SOCKET_NAME);

    m_tag = NULL;
    m_disconnectListener = NULL;
    m_connected = false;
    m_threadStopped = false;
    m_packetSize = 4096;

    bUnixDomain = true;
    strcpy(clientName, "");
}


SocketClient::SocketClient(int socket, const char* name){
    m_socket = socket;
    m_tag = NULL;
    m_disconnectListener = NULL;
    m_connected = true;
    m_threadStopped = false;
    m_packetSize = 4096;
    pthread_create(&m_thread, NULL, &staticReceiveThread, this);
    strcpy(clientName, name);
}

int SocketClient::getSocket(){
    return m_socket;
}

bool SocketClient::connect(){
    if (bUnixDomain){
        m_socket = socket(AF_UNIX , SOCK_STREAM , 0);
        if(m_socket == -1)
        {
            return false;
        }

        if(::connect(m_socket, (struct sockaddr *)&m_unserver, sizeof(m_unserver)) < 0)
        {
            return false;
        }
    }
    else{
        m_socket = socket(AF_INET , SOCK_STREAM , 0);
        if(m_socket == -1)
        {
            return false;
        }

        if(::connect(m_socket, (struct sockaddr *)&m_server, sizeof(m_server)) < 0)
        {
            return false;
        }
    }

    m_connected = true;
    pthread_create(&m_thread, NULL, &staticReceiveThread, this);

    return true;
}

void SocketClient::disconnect(){
    close(m_socket);
    m_connected = false;
    m_threadStopped = true;
}

//TODO: add lock protect.
bool SocketClient::send(std::string message){
    uint32_t length = htonl(message.size());
    if(::send(m_socket, &length, sizeof(length), 0) < 0){
        return false;
    }
    if(::send(m_socket, message.c_str(), message.size(), 0) < 0){
        return false;
    }
    return true;
}
bool SocketClient::send_simple(std::string key, std::string message){
    if(send(key)){
        return send(message);
    }
    return false;
}


bool SocketClient::send(std::string key, std::vector<std::string> messages){
    if(send(key)){
        return send(vectorToString(messages));
    }
    return false;
}

//static SocketClient::receiveSpecifiedLen(

int SocketClient::receive(std::string &message){
    uint32_t length=0;
    int code;


    code = ::recv(m_socket, &length, sizeof(length), MSG_WAITALL);


//    uint32_t length1 = ntohl(length);
    if(code!=-1 && code!=0){
        length = ntohl(length);

    cout<<code << " , " << length << endl;

        char server_reply[length];
        message = "";

        for(int i=0 ; i<length/m_packetSize ; i++){
            code = ::recv(m_socket, server_reply, m_packetSize, 0);
            if(code!=-1 && code!=0){
                message += std::string(server_reply, m_packetSize);
            }
            else{
                return code;
            }
        }
        if(length%m_packetSize!=0){
            char server_reply_rest[length%m_packetSize];
            code = ::recv(m_socket, server_reply_rest, length%m_packetSize, 0);
            if(code!=-1 && code!=0){
                message += std::string(server_reply_rest, length%m_packetSize);
            }
        }
        cout<<"msg = " <<message << endl;
    }
    return code;
}

int SocketClient::receive_buf(void* buf, uint32_t length){
    int code;
    int received = 0;

    while(received < length){
        code = ::recv(m_socket, (char*)buf + received, length-received, 0);
        if(code < 0){ //TODO
            printf("%s: fail to recv buf, ret=%d\n", __FUNCTION__, code);
            return code;
        }
        if(code == 0){
            printf("%s: fail to recv buf, ret=%d\n", __FUNCTION__, code);
            return code;
        }
        received += code;
    }
    if (received != length){
        printf("%s: request receive=%d, real=%d\n", __FUNCTION__, length, received);
        return 0; //let call disconnect socket
    }
    return received; //return the real received count
}

void SocketClient::addListener(std::string key, void (*messageListener) (SocketClient*, std::string)){
    m_messageListenerMap[key] = messageListener;
}

void SocketClient::setDisconnectListener(void (*disconnectListener) (SocketClient*)){
    m_disconnectListener = disconnectListener;
}

void SocketClient::setTag(void *tag){
    m_tag = tag;
}

void* SocketClient::getTag(){
    return m_tag;
}

void SocketClient::receiveThread(){
    std::string key, message;
    int code1, code2;
    while (!m_threadStopped) {
        code1 = receive(key);

//        if(code1==0){
//            disconnect();
//            if(m_disconnectListener!=NULL){
//                (*m_disconnectListener)(this);
//            }
//        }

        code2 = receive(message);
        if(code1==0 || code2==0){
            disconnect();
            if(m_disconnectListener!=NULL){
                (*m_disconnectListener)(this);
            }
        }
        else if(code1!=-1 && code2!=-1){
            if(m_messageListenerMap[key]!=NULL){
                //(*m_messageListenerMap[key])(this, stringToVector(message));
                (*m_messageListenerMap[key])(this, message);
            }
        }
    }
}

#if 0

struct pt_data {
    struct vdIn *ptvideoIn;
    float frmrate;
} ptdata;
void SocketClient::receiveThread_ext(){
    //DEBUG
    //FILE* dump=fopen("streamsocket.raw", "wb");
    //
    std::string key, message;
    int code1, code2, code3;
    uint32_t msgId = 0;
    char* frame = NULL;
	const int frmrate_update = 30;

    //measure fps
    struct timeval tv;
    gettimeofday(&tv,NULL);
    long lasttime_ms = tv.tv_sec*1000 + tv.tv_usec/1000;
	int loop_counter = 0;
    long currtime_ms;

	float frmrate = 0.0;		// Measured frame rate

    while (!m_threadStopped) {
        //receive msg head
        code1 = receive_buf(&msgId, sizeof(msgId));
        if (msgId == MSG_FRAME_INFO){
            struct MsgFrameInfo msg;
            memset(&msg, 0, sizeof(msg));
            code2 = receive_buf(&msg, sizeof(msg));
#ifdef DEBUG
            printf("%s: %s: receive msgId=%d, wxh=%dx%d, buflen=%d\n", __FUNCTION__, clientName, msgId, msg.width, msg.height, msg.bufLen);
#endif
            if (frame == NULL){
                frame = (char*)malloc(msg.bufLen);
                if (frame == NULL){
                    printf("%s: %s: ERROR: fail to mallc frame buffer with size %d\n", __FUNCTION__, clientName, msg.bufLen);
                    return;
                }
            }

            code3 = receive_buf(frame, msg.bufLen);

		    if(loop_counter ++ % frmrate_update == 0){
                gettimeofday(&tv,NULL);
                currtime_ms = tv.tv_sec*1000 + tv.tv_usec/1000;
                if (currtime_ms - lasttime_ms > 0){
                    frmrate = frmrate_update * (1000.0 / (currtime_ms - lasttime_ms));
                }
                lasttime_ms = currtime_ms;

			    printf("%s: frame rate: %g \n", clientName, frmrate);
            }
        }
        else if (msgId == MSG_CLIENT_NAME){
            //get client name
            struct MsgClientName msg;
            memset(&msg, 0, sizeof(msg));
            code2 = receive_buf(&msg, sizeof(msg));
            code3 = 1; //TODO: avoid disconnect
            strcpy(clientName, msg.name);
            printf("%s: client name: %s\n", __FUNCTION__, clientName);
        }
        else{
            printf("%s: %s: ERROR: unrecognized msgid %d\n", __FUNCTION__, clientName, msgId);
            code1 = code2 = code3 = 0; //TODO
        }
        if(code1==0 || code2==0 || code3==0){
            printf("%s: %s: disconnect socket\n", clientName, __FUNCTION__);
            disconnect();
            if(m_disconnectListener!=NULL){
                (*m_disconnectListener)(this);
            }
        }
        else if(code1!=-1 && code2!=-1 && code3 != -1){
            if(m_messageListenerMap[key]!=NULL){
               // (*m_messageListenerMap[key])(this, stringToVector(message));
            }
        }
    }
    //DEBUG
    //fclose(dump);
    free(frame);
}
#endif
