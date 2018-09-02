#ifndef MESSAGE_H
#define MESSAGE_H

#define DEBUG
//#undef DEBUG

//#define USE_UNIX_DOMAIN

#ifdef USE_UNIX_DOMAIN
#warning "USE UNIX DOMAIN SOCKET"
#endif

//for uxin domain
#define UNIX_DOMAIN_SOCKET_NAME "ipc_socket"
//for inter domain
//#define SOCKET_IP_ADDR "127.0.0.1"
#define SOCKET_IP_ADDR "192.168.0.100"
//#define SOCKET_IP_ADDR "67.218.158.111"
//#define SOCKET_IP_ADDR "xm-server"
#define SOCKET_PORT (8882)

//for socket tranfer config
#define MAX_STR_SIZE  1024   //数据包的长度
#define MSG_INVALID (0x00FF)


#define MSG_FRAME_INFO (1)
#define MSG_CLIENT_NAME (2)
#define MSG_TRANSFER_STR (3)

struct MsgFrameInfo{
    int format;
    int width;
    int height;
    int bufLen;
};
typedef struct MsgFrameInfo msgFrameInfo_t;
struct MsgClientName{
    char name[64];
};
typedef struct MsgClientName msgClientName_t;

struct MsgTransferString{
    char str[MAX_STR_SIZE];
};

#endif
