#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <glog/logging.h>

#define WEIGHT_PORT 7112
#define AD_CLIENT_PORT 8113
#define BUFFER_LENGTH 1024
#define MAX_CONN_LIMIT 512

#define NUM_OF_SENSORS 1024
#define NUM_OF_CLIENTS 512

#define TIMEOUT_TIME 20

typedef struct MysocketInfo{
    int socketCon;
    char *ipaddr;
    uint16_t port;
}_MySocketInfo;


typedef struct productInfo{
    int sockfd;
    int id; //sensor id
    int weights; //sensor weights
    int isView; // 1-->View
    int count; // numeber of views
    int isInit;
    int loop; // if loop>100, goods lost
    int lost;
    float goodsErr; //weight error

}_productInfo;


typedef struct ipMacInfo{
    char ip[20];
    char mac[20];
    int sockfd;
}_ipMacInfo;

typedef struct adClientMacInfo{
    char mac[20];
    int sockfd;
}_AdClientInfo;

struct productInfo product[NUM_OF_SENSORS];
struct ipMacInfo ipMac[NUM_OF_CLIENTS];
struct adClientMacInfo adClient[NUM_OF_CLIENTS];

char* cutStringSaveinArray(char* f_dest,char* f_source,const char* f_start,const char* f_end,int f_destbuff_len);
void sensorData(int fd, char *recvData);
void ADData(int fd, char *recvData);
static void *funThrWeightRecvHandler(void *sock_fd);
void resetProductStatus(int fd);


static void *thrAdServer(void *);
static void *thrWeightServer(void *);
void send2AdClient(char *adMac, int adId);
static void *funThrAdRecvHandler(void *sock_fd);
int resetAdFd(int fd);
void saveMac(int fd, char *recvData);
void saveFd(int fd, char *recv_data);

int main(int argc ,char *argv[])
{

    google::InitGoogleLogging(argv[0]);

    pthread_t thrWeightId, thrAdId;
    if(pthread_create(&thrWeightId, NULL, thrWeightServer, NULL) == -1)
    {
        LOG(ERROR) << "thrWeight_create error!";
    } 
    if(pthread_create(&thrAdId, NULL, thrAdServer, NULL) == -1 )
    {
        LOG(ERROR) << "thrAdServer_create error!";
    } 
    while(1)
    {
    }

}


static void *thrAdServer(void *)
{
    int sockfd_server;
    int sockfd;
    struct sockaddr_in s_addr_in;
    struct sockaddr_in s_addr_client;
    int client_length;

    LOG(INFO) << "Start AD server...";

    sockfd_server = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd_server == -1)
    {
        LOG(ERROR) << "socket error!";
        return NULL;
    }

    //before bind(),set the attr of structure sockaddr.
    memset(&s_addr_in, 0, sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr_in.sin_port = htons(AD_CLIENT_PORT);


    //before bind(),set the attr of structure sockaddr.
    memset(&s_addr_in, 0, sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr_in.sin_port = htons(WEIGHT_PORT);

    LOG(INFO) << "bind...";
    int fd_temp = bind(sockfd_server, (struct sockaddr*)(&s_addr_in), sizeof(s_addr_in));
    if(fd_temp == -1)
    {
        LOG(ERROR) << "bind error!";
        return NULL;
    }

    LOG(INFO) << "listen...";
    if(listen(sockfd_server, MAX_CONN_LIMIT) == -1)
    {
        LOG(ERROR) << "listen error!";
        return NULL;
    }

    while(1){
        LOG(INFO) << "waiting for new connection...";
        printf("waiting for new connection...\n");
        pthread_t thread_id;
        client_length = sizeof(s_addr_client);

        //Block here, until server accept a new connection
        sockfd = accept(sockfd_server, (struct sockaddr*)(&s_addr_client), (socklen_t *)(&client_length));
        if(sockfd == -1)
        {
            LOG(ERROR) << "Accept error!";
            continue;
        }

        LOG(INFO) << "A new connection occurs...";
        
        printf("A new connection occurs...\n");
        if(pthread_create(&thread_id, NULL, funThrAdRecvHandler, (void*)(&sockfd)) == -1)
        {
            LOG(ERROR) << "pthread_create error!";
            break;
        } 

    }
    //Clear
    int ret = shutdown(sockfd_server, SHUT_WR);
    assert(ret != -1);

    LOG(INFO) << "Ad Server shutdown!";
    return NULL;

}


static void *thrWeightServer(void *)
{
    int sockfd_server;
    int sockfd;

    struct sockaddr_in s_addr_in;
    struct sockaddr_in s_addr_client;
    int client_length;
    
    //google::ParseCommandLineFlags(&argc, &argv, true);
    LOG(INFO) << "Start Weight server...";
    sockfd_server = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd_server == -1)
    {
        LOG(ERROR) << "socket error!";
        return NULL;
    }

    //before bind(),set the attr of structure sockaddr.
    memset(&s_addr_in, 0, sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr_in.sin_port = htons(WEIGHT_PORT);

    LOG(INFO) << "bind...";
    int fd_temp = bind(sockfd_server, (struct sockaddr*)(&s_addr_in), sizeof(s_addr_in));
    if(fd_temp == -1)
    {
        LOG(ERROR) << "bind error!";
        return NULL;
    }

    LOG(INFO) << "listen...";
    if(listen(sockfd_server, MAX_CONN_LIMIT) == -1)
    {
        LOG(ERROR) << "listen error!";
        return NULL;
    }

    while(1)
    {
        LOG(INFO) << "waiting for new connection...";
        printf("waiting for new connection...\n");
        pthread_t thread_id;
        client_length = sizeof(s_addr_client);

        //Block here, until server accept a new connection
        sockfd = accept(sockfd_server, (struct sockaddr*)(&s_addr_client), (socklen_t *)(&client_length));
        if(sockfd == -1)
        {
            LOG(ERROR) << "Accept error!";
            continue;
        }

        LOG(INFO) << "A new connection occurs...";
        
        printf("A new connection occurs...\n");
        if(pthread_create(&thread_id, NULL, funThrWeightRecvHandler, (void*)(&sockfd)) == -1)
        {
            LOG(ERROR) << "pthread_create error!";
            break;
        } 

    }

    //Clear
    int ret = shutdown(sockfd_server, SHUT_WR);
    assert(ret != -1);

    LOG(INFO) << "Weight Server shutdown!";
    return NULL;
}


void send2AdClient(char *adMac, int adId)
{
    LOG(INFO) << "send data to AD Client...";
    for(int i=0; i<NUM_OF_CLIENTS; i++)
    {
        if(strcmp(adClient[i].mac, adMac) == 0)
        {
            if((send(adClient[i].sockfd, &adId, sizeof(adId), 0)) < 0)
            {
                LOG(ERROR) << "send adId error..";
            }
            break;
        }
    }
    
}

static void *funThrAdRecvHandler(void *sock_fd)
{
    int fd = *((int *)sock_fd);
    int i_recvBytes;
    char data_recv[BUFFER_LENGTH];

    while(1){
        LOG(INFO) << "recv AD client data...";
        memset(data_recv, 0, BUFFER_LENGTH);

        i_recvBytes = recv(fd, data_recv, BUFFER_LENGTH, 0);
        printf("recv data:%s\n", data_recv);
        if(i_recvBytes == 0)
        {
            LOG(INFO) << "Client has closed!";
            printf("client has closed\n");
            break;
        }
        else if(i_recvBytes == -1)
        {
            LOG(ERROR) << "recv error";
            break;
        }
        //data from AD client
        if(strstr(data_recv, "register") != NULL)
        {
            saveFd(fd, data_recv);
        }
        if(strstr(data_recv, "mac") != NULL)
        {
            saveMac(fd, data_recv);
        }
        
    }    
    //Clear
    LOG(INFO) << "terminating current AD client_connection...";
    printf("terminating current AD client_connection...\n");
    resetAdFd(fd);
    pthread_exit(NULL);
}

int resetAdFd(int fd)
{
    for(int i=0; i<NUM_OF_CLIENTS; i++)
    {
        if(adClient[i].sockfd == fd)
        {
            adClient[i].sockfd = 0;
            memset(adClient[i].mac, 0, sizeof(adClient[i].mac));
            break;
        }
    }
    close(fd);
}

void saveMac(int fd, char *recvData)
{
    char mac[20];
    memset(mac, 0 ,sizeof(mac));
    cutStringSaveinArray(mac, recvData, "mac:", "\0", 20);
    for(int i=0; i<NUM_OF_CLIENTS; i++)
    {
        if(adClient[i].sockfd == fd)
        {
            strcpy(adClient[i].mac, mac);
            break;
        }
    }
}

void saveFd(int fd, char *recv_data)
{
    for(int i=0; i<NUM_OF_CLIENTS; i++)
    {
        if(adClient[i].sockfd == 0)
        {
            adClient[i].sockfd = fd;
            break;
        }
    }
}


static void *funThrWeightRecvHandler(void *sock_fd)
{
    int fd = *((int *)sock_fd);
    int i_recvBytes;
    char data_recv[BUFFER_LENGTH];
    const char *data_send = "TODO";
    //printf("Recv Func..\n");
    //超时设置
    struct timeval timeout;
    int result;
    socklen_t len = sizeof(timeout);
    timeout.tv_sec = TIMEOUT_TIME;
    timeout.tv_usec = 0;

    while(1)
    {
        LOG(INFO) << "waiting for request...";
        printf("waiting for request...\n");
        //Reset data
        memset(data_recv, 0, BUFFER_LENGTH);

        result = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout.tv_sec, len);
        printf("reslut=%d\n", result);
        if(result < 0)
        {
            printf("timeout.\n");
            resetProductStatus(fd);
            break;    
        }

        i_recvBytes = recv(fd, data_recv, BUFFER_LENGTH, 0);
        printf("recv data:%s\n", data_recv);
        if(i_recvBytes == 0)
        {
            LOG(INFO) << "Client has closed!";
            printf("client has closed\n");
            resetProductStatus(fd);
            break;
        }
        else if(i_recvBytes == -1)
        {
            LOG(ERROR) << "recv error";
            resetProductStatus(fd);
            break;
        }

        //data from sensor
        if(strstr(data_recv, "id:") !=NULL)
        {
            sensorData(fd, data_recv);
        }
        


    }
    //Clear
    LOG(INFO) << "terminating current client_connection...";
    printf("terminating current client_connection...\n");
    close(fd);
    pthread_exit(NULL);

}


void resetProductStatus(int fd)
{
    for (int i=0; i<NUM_OF_SENSORS; i++)
    {
        if(product[i].sockfd == fd)
        {
            product[i].isView = 0;
            //TODO: sockfd = 0 ?
            break;
        }
    }

}


void sensorData(int fd, char *recvData) 
{
    LOG(INFO) << "Receive data:" << recv; 
    char tmp_id[20];
    char tmp_weights[20];
    memset(tmp_id, 0, sizeof(tmp_id));
    memset(tmp_weights, 0, sizeof(tmp_weights));

    cutStringSaveinArray(tmp_id, recvData, "id:", ",", 20);
    printf("id: %s\n", tmp_id);
    cutStringSaveinArray(tmp_weights, recvData, "weights:", "\0", 20);
    printf("weight: %s\n", tmp_weights);

    int sensor_id = atoi(tmp_id);
    int weights = atoi(tmp_weights);

    //TODO: read data from MySQL

    //products table: ad_id,ad_mac

    //layers table: sensor_id, product_id, quantity, weight, avg_weight, 

#if 1    
    int j=0;
    for(j=0; j<NUM_OF_SENSORS; j++)
    {
        if(product[j].id == sensor_id)
        {
            break;
        }
    }
    printf("j=%d\n", j);
    if(j >= NUM_OF_SENSORS)
    {
        //Initialize the product
        for(int i=0; i<NUM_OF_SENSORS; i++)
        {
            if(product[i].isInit == 0)
            {
                product[i].sockfd = fd;
                product[i].id = sensor_id;
                product[i].weights = weights;            
                product[i].isView = 0;
                product[i].count = 0;
                product[i].isInit = 1;
                break;
            }
        }
    }
    else
    {
        if(product[j].isView == 0)
        {
            if(weights < 0)
            {
                product[j].isView = 1;
                product[j].count += 1;
                product[j].sockfd = fd;
                printf("view count:%d\n", product[j].count);
                //TODO:update MySQL, send AD ID to AD client.
#if 1
                char mac[20];
                memset(mac, 0, sizeof(mac));
                strcpy(mac, "aa:bb:cc:dd:ee:ff");
                int adId = 35;
#endif
                send2AdClient(mac, adId);

            }
        }   
        else
        {
            if(weights < 0)
            {
                //商品长时间未放置回原处
                product[j].loop += 1;
                return ;
            }
        }
    }
    
    
    //商品丢失
    if(product[j].loop > 100)
    {
        printf("warning: the goods lost!!!");
        product[j].lost = 1;
        //TODO:update MySQL
    }


    //补货后，重置丢失状态
    if((weights > 0) && (product[j].loop > 100))
    {
        product[j].lost = 0;
        product[j].loop = 0;
        //TODO:updata MySQL

    }



#endif    


}


char* cutStringSaveinArray(char* f_dest,char* f_source, const char* f_start,const char* f_end, int f_destbuff_len)
{
	char* cursor_head=NULL;
	char* cursor_tail=NULL;
	int nlen;

	if(f_source==NULL) return NULL;
	cursor_head=strstr(f_source,f_start);
	
	if(cursor_head==NULL)
	{
		*f_dest='\0';
		return NULL;
	}
	
	nlen=strlen(f_start);
	cursor_head+=nlen;
	if(f_end==NULL||strcmp(f_end,"")==0) cursor_tail=f_source+strlen(f_source);
	else cursor_tail=strstr(cursor_head,f_end);
	if(cursor_tail==NULL)
	{
		*f_dest='\0';
		return NULL;
	}
	nlen=cursor_tail-cursor_head;
	if(f_destbuff_len<=nlen) nlen=f_destbuff_len-1;

	strncpy(f_dest,cursor_head,nlen);
	*(f_dest+nlen)='\0';
	if(f_end==NULL) return cursor_tail;
	else return cursor_tail+strlen(f_end);
}


int _System(const char * cmd, char *pRetMsg, int msg_len)
{
	FILE * fp;
	char * p = NULL;
	int res = -1;
    int i = 0;
	if (cmd == NULL || pRetMsg == NULL || msg_len < 0)
	{
		printf("Param Error!\n");
		return -1;
	}
	if ((fp = popen(cmd, "r") ) == NULL)
	{
		printf("Popen Error!\n");
		return -2;
	}
	else
	{
		memset(pRetMsg, 0, msg_len);
		//get lastest result
		while(fgets(pRetMsg, msg_len, fp) != NULL)
		{
            char mac[20];
            char ip[20];
            memset(ip, 0, sizeof(ip));
            memset(mac, 0, sizeof(mac));
            cutStringSaveinArray(ip, pRetMsg, "IP:[", "]", 20);
            cutStringSaveinArray(mac, pRetMsg, "MAC:[", "]", 20);
            //TODO: write to strcut ipMACInfo
            memset(ipMac[i].ip, 0, 20);
            memset(ipMac[i].mac, 0, 20);
            strcpy(ipMac[i].ip, ip);
            strcpy(ipMac[i].mac, mac);
            i++;
		}
 
		if ( (res = pclose(fp)) == -1)
		{
			printf("close popenerror!\n");
			return -3;
		}
		pRetMsg[strlen(pRetMsg)-1] = '\0';
		return 0;
	}
}

