#ifndef SERVER_H
#define SERVER_H


#include "Tools.h"
#include "lib/SocketClient.h"
#include "lib/SocketServer.h"

//TODO: add lock for client send
int send_adid_to_client_bymac(const string mac, const string ad);
int send_adid_to_client(SocketClient* client, const string ad);
int send_adid_to_allclients(const string& ad);
SocketClient* getClientByMac(const string& mac);
void onMessage_register(SocketClient *socket, string message);
void onMessage(SocketClient *socket, string message);
void onDisconnect(SocketClient *socket);
void freeMemory();
int startServer();
int stopServer();

#endif
