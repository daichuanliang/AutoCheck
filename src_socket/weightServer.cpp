#include <unistd.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <thread>
#include <string>
#include <mutex>
#include <glog/logging.h>
#include <stdlib.h>
#include "sql.h"


#include "Tools.h"
#include "lib/SocketClient.h"
#include "lib/SocketServer.h"
#include "weightServer.h"

#ifdef ENABLE_JSON_POST
//#include "jsonAPI.h"
#include "curlPost.h"
#endif


#define TIMEOUT_TIME 20
#define WEIGHT_PORT 7112
#define AD_CLIENT_PORT 8113
#define BUFFER_LENGTH 1024
#define MAX_CONN_LIMIT 512

#define NUM_OF_SENSORS 1024
#define NUM_OF_CLIENTS 512
#define SQLLEN 256

#define LOST_WARNING_INTERATION 20

using namespace std;
float DEFAULT_W = 955;
int GOODS_COUNT = 2;
float  GOODS_HALF = ((DEFAULT_W/GOODS_COUNT)/2);
float  GOODS_W = (DEFAULT_W/GOODS_COUNT);
float DELTA = 50;
static std::vector<SocketClient*> clientsVector;
static std::mutex mtx_lock;
bool isView = false;
int view_count = 0;
float process_delta = 0;
uint32_t loop = 0;
int ad_id = 30;
//TODO: add lock for client send



typedef struct productInfo{
    int sockfd;
    int id; //sensor id
    float weights; //sensor weights
    int isView; // 1-->View
    int count; // numeber of views
    int isInit;
    int loop; // if loop>100, goods lost
    int lost;
    int quantity; //goods quantity
    int adId; // AD  id
    float aveWeight;
    char mac[20]; //TODO: Change 1 mac to Multiple macs.
    float goodsErr; //weight error range
    int storeId;
    int sendStatusToWeb;
}_productInfo;

struct productInfo product[NUM_OF_SENSORS];
MYSQL *conn;

void resetProductStatus(int fd);
char* cutStringSaveinArray(char* f_dest,char* f_source, const char* f_start,const char* f_end, int f_destbuff_len);
void sensorData(int fd, char *recvData);
int readDataFromMySQL(MYSQL *conn);
static int setTimeout(int fd);
int startWeightWebServer();

static int send_adid_to_client(SocketClient* client, const string ad){

    cout<<__FUNCTION__<<":"<<ad<<endl;
    if (client == nullptr){
        cout<<__FUNCTION__<<": null client, skip"<<endl;
        return 0;
    }

    string key = "advertise";
    vector<string> ads;
    ads.push_back(ad);
    client->send_simple(key, ad);
    return 0;
}

static int send_adid_to_allclients(const string& ad)
{
    std::vector<SocketClient*> clients;
    {
        lock_guard<mutex> l(mtx_lock);
        clients = clientsVector;
    }
    for (auto& c:clients) {
        send_adid_to_client(c, ad);
    }
}

//get socket client instance by client's mac
static SocketClient* getClientByMac(const string& mac) {
    lock_guard<mutex> l(mtx_lock);

    for (auto& c:clientsVector) {
        const string& clientMac = c->getMac();
        cout<<__FUNCTION__<<"clientMac="<<clientMac<<", mac="<<mac<<endl;
        if (mac.find(clientMac) != string::npos){
            cout<<__FUNCTION__<<"found existing client with mac : "<< mac<<endl;
            return c;
        }
    }
    return nullptr;
}

static int send_adid_to_client_bymac(const string mac, const string ad){
    cout<<__FUNCTION__<<__LINE__<<endl;
    SocketClient* client = getClientByMac(mac);
    cout<<__FUNCTION__<<__LINE__<<endl;
    send_adid_to_client(client, ad);
    cout<<__FUNCTION__<<__LINE__<<endl;
    return 0;
}

static void forward(string key, vector<string> messages, SocketClient *exception){
	std::string *_uid = (std::string*) exception->getTag();
	for(auto x : clientsVector){
		std::string *uid = (std::string*) x->getTag();
		if((*uid)!=(*_uid)){
			x->send(key, messages);
		}
	}
}

static void onMessage_register(SocketClient *socket, string message){
    cout<<"server receive client message: register " <<endl;
    if (message.size() <= 0){
        cout<<"Invalid messages size"<<endl;
        return;
    }
    transform(message.begin(), message.end(), message.begin(), ::tolower);
    socket->setMac(message);
    cout<<"->"<<socket<<" : " << *(string*)socket->getTag() << " : " <<message<<endl;
}

static void onMessage(SocketClient *socket, string message){
    cout<<"server receive client message" <<message<<endl;
}

static void onDisconnect(SocketClient *socket){
	cout << "client disconnected !" << endl;
	//forward("message", {"Client disconnected"}, socket);
	std::string *_uid = (std::string*) socket->getTag();
    lock_guard<mutex> l(mtx_lock);
	for(int i=0 ; i<clientsVector.size() ; i++){
		std::string *uid = (std::string*) clientsVector[i]->getTag();
		if((*uid)==(*_uid)){
			clientsVector.erase(clientsVector.begin() + i);
            cout<<"OnDisconnect handle client: " << *_uid <<endl;
            break; //should not continue the loop, with erase ops
		}
	}
	delete socket;
}

static void freeMemory(){
	for(auto x : clientsVector){
		delete (std::string*) x->getTag();
		delete x;
	}
}

static int setTimeout(int fd){
	struct timeval timeout;
    int result;
    socklen_t len = sizeof(timeout);
    timeout.tv_sec = TIMEOUT_TIME;
    timeout.tv_usec = 0;
	result = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout.tv_sec, len);
	
	//printf("reslut=%d\n", result);
	if(result < 0)
    {
		printf("timeout.\n");
        resetProductStatus(fd);
        return result;    
    }
	return 0;
}

int startWeightWebServer() {
	srand(time(NULL));
    uint32_t length = 100;
    char buf[length];
    memset(buf, 0, length);
#ifdef USE_UNIX_DOMAIN
    //use unix domain socket
	SocketServer server;
    cout<<"server use unix domain socket " << UNIX_DOMAIN_SOCKET_NAME<<endl;
#else
    //use inter domain socket
	SocketServer server(8118);
    cout<<"server use inter domain socket "<< SOCKET_IP_ADDR << " : 8118"  << endl;
#endif

	if(server.start()){
		while (1) {
			int sock = server.accept();
			if(sock!=-1){                
                memset(buf, 0, length);
				cout << "weight Web connected !" << endl;
				int len = ::recv(sock, buf, length, 0);
                 cout<<"weight Web send data ="<<buf<<endl;
				if(strstr(buf, "InitDone") != NULL)
				{
					//clear productInfo
					for(int i=0; i<NUM_OF_SENSORS; i++)
					{
						product[i].adId = 0;
						memset(product[i].mac, 0, sizeof(product[i].mac));
						product[i].id = 0;
						product[i].quantity = 0;
						product[i].weights = 0;
						product[i].aveWeight = 0;
						product[i].goodsErr = 0;
					}
					//Init data from MySQL
					string status = "OK";
					#if 1
					int sts=readDataFromMySQL(conn);
					if (sts<0){
						status = "Fail";
					}
					else{
						status = "OK";
					}
					#endif
					//send OK to Weight Web
					if(send(sock, status.c_str(), status.size(), 0) < 0){
						cout << "send data to Weights Web error!!!!" << endl;
					}
					//mysql_close(conn);
					
				}
				else if(strstr(buf, "store_id:") != NULL)
				{
					cout << "init Web data" << endl;
					//TODO
					initWebData(buf, sock);
				}
				
			}
		}
	}
	
}

void initWebData(char *recvData, int fd) 
{
    cout << "recvData from Web:" << recvData <<endl;
    char store_id[10];
    char sensor_id[10];
    memset(store_id, 0, sizeof(store_id));
    memset(sensor_id, 0, sizeof(sensor_id));
    cutStringSaveinArray(store_id, recvData, "store_id:", ",", 10);
    cutStringSaveinArray(sensor_id, recvData, "sensor_id:", "\0", 10);

    cout << "store_id:" << store_id << "   sensor_id:" <<sensor_id << endl;
    string sendData;
    char tmp[BUFFER_LENGTH*10];
    int i = 0;
    int j = 0;
    memset(tmp, 0, sizeof(tmp));
    if(atoi(sensor_id) != 0)
    {
        for(i=0; i<NUM_OF_SENSORS; i++)
        {
            if((product[i].id == atoi(sensor_id)) && (product[i].storeId == atoi(store_id)))
            {
                sprintf(tmp, "[{\"store_id\":%s, \"sensor_id\":%s, \"weight\":%f}]", store_id, sensor_id, product[i].weights);
                cout << "tmp:" << tmp <<endl;
                sendData = tmp; 
                break;
            }
        }
    }
    else
    {
        char tmp_head[10];
        char tmp_data[BUFFER_LENGTH*10];
        memset(tmp_head, 0, sizeof(tmp_head));
        memset(tmp_data, 0, sizeof(tmp_data));
        sprintf(tmp_head, "[");

        for(j=0; j<NUM_OF_SENSORS; j++)
        {
            if(product[j].storeId == atoi(store_id))
            {
                sprintf(tmp_data, "%s{\"store_id\":%s, \"sensor_id\":%d, \"weight\":%f},", tmp_data, store_id, product[j].id, product[j].weights); 
            }
        }
        cout << "tmp_data:" <<tmp_data<<endl;
        sprintf(tmp, "%s%s]", tmp_head, tmp_data);
        sendData = tmp; 
        sendData.erase(sendData.end() - 2);
        cout << "send data to Web 111:" << sendData << endl;
    }
    cout << "i:" << endl;
    if(i>=NUM_OF_SENSORS)
    {
        sendData = "InitFailed";
    }
    cout << "send data to Web:" << sendData << endl;
	if(send(fd, sendData.c_str(), sendData.size(), 0) < 0){
		cout << "send data to Weights Web error!!!!" << endl;
	}

}


int startWeightServer() {
	srand(time(NULL));
    uint32_t length = 100;
    char buf[length];
    memset(buf, 0, length);
#ifdef USE_UNIX_DOMAIN
    //use unix domain socket
	SocketServer server;
    cout<<"server use unix domain socket " << UNIX_DOMAIN_SOCKET_NAME<<endl;
#else
    //use inter domain socket
	SocketServer server(7112);
    cout<<"server use inter domain socket "<< SOCKET_IP_ADDR << " : 7112"  << endl;
#endif

	if(server.start()){
		while (1) {
			int sock = server.accept();
			int sts = setTimeout(sock);
			if(sts < 0){
				resetProductStatus(sock);
			}
			if(sock!=-1){
				cout << "weight client connected !" << endl;
                while(1) {
                    int len = ::recv(sock, buf, length, 0);
                    cout<<"weight sensor buf ="<<buf<<endl;
					if(len <= 0)
					{
						LOG(INFO) << "Client has closed!";
						cout << "Client has closed!" << endl;
						resetProductStatus(sock);
						//break;
					}
					//data from sensor
					if(strstr(buf, "id:") !=NULL)
					{
						sensorData(sock, buf);
					}
					
					
					#if 0
                    char* pos = NULL;
                    pos = strchr(buf, ',');
                    if (pos != NULL) {
                        char* pos2 = NULL;
                        pos2 = strchr(buf, ':');
                        if (pos2 != NULL) {
                            char tmp1[100];
                            memset(tmp1, 0, 100);
                            int id_len = pos - pos2;
                            id_len = id_len - 1;
                            memcpy(tmp1, pos2 + 1, id_len);
                            int id = atoi(tmp1);
                            cout<< "sensor id ="<<id<<endl;
                            pos2 = strchr(pos+1, ':');
                            if (pos2 != NULL) {
                                float weight = atof(pos2+1);
                                cout<< "weight ="<<weight<<endl;
                                if (isView == true) {
                                    loop++;
                                }

                                if (loop > 100) {
                                    cout<<"warning the goods lost!!!!!!!!!!!"<<endl;
                                }

                                if (( fabs(weight + GOODS_W)< DELTA) && !isView) {
                                    isView = true;
                                    view_count++;
                                    cout<<"view++"<<endl;
                                    process_delta = 1;
                                    send_adid_to_allclients(to_string(ad_id));
                                    continue;
                                }
                                if (isView) {

                                    if ((fabs(weight) + DELTA)/GOODS_W > process_delta) {
                                        process_delta = (fabs(weight) + DELTA)/GOODS_W;
                                        view_count++;
                                        cout<<"isView = true"<<endl;
                                        send_adid_to_allclients(to_string(ad_id));
                                    }

                                }

                                cout<<"view count = "<<view_count<<endl;
                            }
                        }
                    }
					#endif
                }
			}
		}
	}
	else{
        perror("start server error");
		cout << "fail to start weight server" << endl;
	}

    //in most case, will not enter.
	freeMemory();
    return 0;

}
int startAdServer() {
	srand(time(NULL));

#ifdef USE_UNIX_DOMAIN
    //use unix domain socket
	SocketServer server;
    cout<<"server use unix domain socket " << UNIX_DOMAIN_SOCKET_NAME<<endl;
#else
    //use inter domain socket
	SocketServer server(8113);
    cout<<"server use inter domain socket "<< SOCKET_IP_ADDR << " : 8113" << endl;
#endif

	if(server.start()){
		while (1) {
			int sock = server.accept();
			if(sock!=-1){
				cout << "client connected !" << endl;
				SocketClient *client = new SocketClient(sock);
				client->addListener("message", onMessage);
				client->addListener("register", onMessage_register);

				client->setDisconnectListener(onDisconnect);
				client->setTag(new std::string(getUid()));
				clientsVector.push_back(client);
			}
		}
	}
	else{
        perror("start server error");
		cout << "fail to start ad server 8113" << endl;
	}

    //in most case, will not enter.
	freeMemory();
    return 0;
}

static int stopServer()//TODO
{
    return 0;
}


int main(int argc , char *argv[]){
	
	
	//open MySQL

    conn = mysql_init(NULL);
    connectDatabase(conn, "localhost", "root", "aim_123456", "shopdb");
    readDataFromMySQL(conn);
#if 0
    thread t = thread([](){
        startServer();
    });
#endif
    thread tw = thread([](){
        startWeightServer();
    });
    thread ta = thread([](){
        startAdServer();
    });
	thread tww = thread([](){
        startWeightWebServer();
    });


    std::string line;
    while(1){
        /*
        cout << "input advertise id: ";
        getline(cin, line);
        send_adid_to_allclients(line);
        */
        usleep(1000*100*1);
        //send_adid_to_allclients("test.mp4");
    }

    stopServer();
   // t.join();
    tw.join();
    ta.join();
	tww.join();
	mysql_close(conn);
	return 0;
}


int readDataFromMySQL(MYSQL *conn)
{
    char sql[SQLLEN];
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select ad_id,mac,sensor_id,quantity,weight,ave_weight,product_id, layers.store_id, is_lost from layers,products where layers.product_id = products.id");
    queryDatabase(conn, sql); 
    MYSQL_RES *result = mysql_store_result(conn);
    if(result == NULL)
    {
        finishWithError(conn);
        return -1;
    }

    //int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;
    int i=0;
    while ((row = mysql_fetch_row(result)))
    {
        //TODO: 赋值
        if(row[0] != NULL)
            product[i].adId = atoi(row[0]);
        memset(product[i].mac, 0, sizeof(product[i].mac));
        if(row[1] != NULL)
            strcpy(product[i].mac, row[1]);
        if(row[2] != NULL)
            product[i].id = atoi(row[2]);
        if(row[3] != NULL)
            product[i].quantity = atoi(row[3]);
        if(row[4] != NULL)
            product[i].weights = atof(row[4]);
        if(row[5] != NULL)
        {
            product[i].aveWeight = atof(row[5]);
            product[i].goodsErr = product[i].aveWeight/3;
        }
        //product[i].productId = atoi(row[])
        if(row[7] != NULL)
            product[i].storeId = atoi(row[7]);
        if(row[8] != NULL)
            product[i].lost = atoi(row[8]);
        cout << "aveWeight:" << product[i].aveWeight <<endl;
        cout << "goodsErr:" << product[i].goodsErr <<endl;
        cout << "storeId:" << product[i].storeId <<endl;
        i++;            
    }
    //mysql_close(conn);
    return 0;

}

void resetProductStatus(int fd)
{
    for (int i=0; i<NUM_OF_SENSORS; i++)
    {
        if(product[i].sockfd == fd)
        {
            product[i].isView = 0;
            //TODO: sockfd = 0 ?
            product[i].loop = 0;
            product[i].lost = 0;
            string json = "{\"store_id\":" + to_string(product[i].storeId) + ",\"sensor_id\":" + to_string(product[i].id) + ",\"is_lost\":"+ to_string(0) + "}";
            cout << "Post data:" << json << endl;
            string output;
            curlPostJson(json, url_lost_control, server_port, output);
            break;
        }
    }

}

void sensorData(int fd, char *recvData) 
{
    //LOG(INFO) << "Receive data:" << recv; 
    char tmp_id[20];
    char tmp_weights[20];
    memset(tmp_id, 0, sizeof(tmp_id));
    memset(tmp_weights, 0, sizeof(tmp_weights));

    cutStringSaveinArray(tmp_id, recvData, "id:", ",", 20);
    //printf("id: %s\n", tmp_id);
    cutStringSaveinArray(tmp_weights, recvData, "weight:", "\0", 20);
    //printf("weight: %s\n", tmp_weights);

    int sensor_id = atoi(tmp_id);
    float weights = atof(tmp_weights);
    //read data from MySQL
    //connect Database
#if 0
    MYSQL *conn;
    conn = mysql_init(NULL);
    connectDatabase(conn, "localhost", "root", "cldai-gpu123--", "shopdb");
#endif

#if 0
    //products table: ad_id,ad_mac

    //layers table: sensor_id, product_id, quantity, weight, avg_weight, 
    const char *sql = "select ad_id,mac,sensor_id,product_id,quantity,weight,ave_weight from layers,products where layers.product_id = products.id;";
    //const char *sql = "select * from products";
    queryDatabase(conn, sql); 
    MYSQL_RES *result = mysql_store_result(conn);
    if(result == NULL)
    {
        finishWithError(conn);
    }
    int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result)))
    {
        for(int i = 0; i < num_fields; i++)
        {
            //TODO: 赋值
            printf("%s ", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
#endif

#if 1    
    int j=0;
    for(j=0; j<NUM_OF_SENSORS; j++)
    {
        if(product[j].id == sensor_id && product[j].weights != 0)
        {
            break;
        }
    }
    if(j >= NUM_OF_SENSORS)
    {
        //Initialize the product
        for(int i=0; i<NUM_OF_SENSORS; i++)
        {
            if(product[i].isInit == 0 && product[i].id == sensor_id)
            {
				cout << "Initialize the product" << endl;
                product[i].sockfd = fd;
                //product[i].id = sensor_id;
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
            //在误差范围内
            
            
            if((weights < 0) && (abs(weights) > product[j].goodsErr))
            {
				cout << "browsing the goods... " <<endl;
				cout << "ready to send data to AD Client..." << endl;
				cout << "weights:" << weights << "   abs(weights):" << abs(weights) << "    goodsErr: " << product[j].goodsErr <<endl;
                product[j].isView = 1;
                product[j].count += 1;
                product[j].sockfd = fd;
                printf("view count:%d\n", product[j].count);
                //update MySQL, send AD ID to AD client.
                //string sql = "update products set view_count=" + product[j].count + "where id in(select product_id from layers where sensor_id=" + product[j].id + ")";
                //const char *sql = "update products set view_count=3 where id in(select product_id from layers where sensor_id=101)"
                #if 0
                char sql[SQLLEN];
                memset(sql, 0, sizeof(sql));
                sprintf(sql, "update products set view_count=%d where id in(select product_id from layers where sensor_id=%d)", product[j].count, product[j].id);
                updateDatabase(conn, sql);
                #endif
                string json = "{\"store_id\":" + to_string(product[j].storeId) + ",\"sensor_id\":" + to_string(product[j].id) + "}";
                string json2 = "{\"store_id\":1, \"sensor_id\":1}";
                cout <<endl << "Post data:" << json << endl;
                cout << "Post data2:" << json2 << endl;
                string output;
                curlPostJson(json, url_view_control, server_port, output);
                                
#if 1
                cout << "AD ID:"<< product[j].adId <<"   mac:" << product[j].mac << endl;
#endif
                //send2AdClient(product[j].mac, product[j].adId);
				string mac = product[j].mac;
				transform(mac.begin(), mac.end(), mac.begin(), ::tolower);
				send_adid_to_client_bymac(mac, to_string(product[j].adId));

            }
        }   
        else
        {
            
            if((weights < 0) && (abs(weights) > product[j].goodsErr))
            {
                //商品长时间未放置回原处
				cout << "The goods don't put back to its original place..." <<endl;
                product[j].loop += 1;
                //return ;
            }
			//else if((weights < 0) && (abs(weights) < product[j].goodsErr))
			else if((abs(weights) < product[j].goodsErr))
			{
				//商品放回原处，但上报重量有误差
				cout << "goods has been put back to its original place...." << endl;
                //product[j].lost = 0;
                //product[j].loop = 0;
				resetProductStatus(fd);
			}
        }
    }
    
    //商品丢失
    if(product[j].loop > LOST_WARNING_INTERATION)
    {
        printf("warning: the goods lost!!!\n");
        //update MySQL
#if 0
        char sql[SQLLEN];
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "update products set is_lost=1 where id in(select product_id from layers where sensor_id=%d)", product[j].id);
        updateDatabase(conn, sql);
#endif
        if(product[j].lost == 0)
        {
            string json = "{\"store_id\":" + to_string(product[j].storeId) + ",\"sensor_id\":" + to_string(product[j].id) + ",\"is_lost\":"+ to_string(1) + "}";
            cout << "Post data:" << json << endl;
            string output;
            curlPostJson(json, url_lost_control, server_port, output);
            product[j].lost = 1;
        }

    }


    //补货后，重置丢失状态
    //if((weights > 0) && (abs(weights) > product[j].goodsErr) && (product[j].loop > LOST_WARNING_INTERATION))
    if((weights > 0) && (abs(weights) > product[j].goodsErr) && (product[j].lost == 1))
    {
        //updata MySQL
#if 0
        char sql[SQLLEN];
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "update products set is_lost=0 where id in(select product_id from layers where sensor_id=%d)", product[j].id);
        updateDatabase(conn, sql);                                
#endif
        string json = "{\"store_id\":" + to_string(product[j].storeId) + ",\"sensor_id\":" + to_string(product[j].id) + ",\"is_lost\":"+ to_string(0) + "}";
        cout << "Post data:" << json << endl;
        string output;
        curlPostJson(json, url_lost_control, server_port, output);
        product[j].lost = 0;
        product[j].loop = 0;

    }



#endif    
	//mysql_close(conn);


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
