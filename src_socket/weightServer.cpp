#include <unistd.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <thread>
#include <string>
#include <mutex>

#include "Tools.h"
#include "lib/SocketClient.h"
#include "lib/SocketServer.h"

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
			if(sock!=-1){
				cout << "weight client connected !" << endl;
                while(1) {
                    int len = ::recv(sock, buf, length, 0);
                    cout<<"weight sensor buf ="<<buf<<endl;
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

#ifdef DebugMain
int main(int argc , char *argv[]){
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

	return 0;
}
#endif

