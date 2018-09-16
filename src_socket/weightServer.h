#ifndef WEIGHT_SERVER_H
#define WEIGHT_SERVER_H


#include "Tools.h"
#include "lib/SocketClient.h"
#include "lib/SocketServer.h"

int startAdServer();
int startWeightServer();
void initWebData(char *recvData, int fd);

#endif
