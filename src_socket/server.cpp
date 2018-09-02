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

std::vector<SocketClient*> clientsVector;
static std::mutex mtx_lock;

//TODO: add lock for client send
int send_adid_to_client(SocketClient* client, const string ad){

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

int send_adid_to_allclients(const string& ad)
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
SocketClient* getClientByMac(const string& mac) {
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

int send_adid_to_client_bymac(const string mac, const string ad){
    cout<<__FUNCTION__<<__LINE__<<endl;
    SocketClient* client = getClientByMac(mac);
    cout<<__FUNCTION__<<__LINE__<<endl;
    send_adid_to_client(client, ad);
    cout<<__FUNCTION__<<__LINE__<<endl;
    return 0;
}

void forward(string key, vector<string> messages, SocketClient *exception){
	std::string *_uid = (std::string*) exception->getTag();
	for(auto x : clientsVector){
		std::string *uid = (std::string*) x->getTag();
		if((*uid)!=(*_uid)){
			x->send(key, messages);
		}
	}
}

void onMessage_register(SocketClient *socket, string message){
    cout<<"server receive client message: register " <<endl;
    if (message.size() <= 0){
        cout<<"Invalid messages size"<<endl;
        return;
    }
    transform(message.begin(), message.end(), message.begin(), ::tolower);
    socket->setMac(message);
    cout<<"->"<<socket<<" : " << *(string*)socket->getTag() << " : " <<message<<endl;
}

void onMessage(SocketClient *socket, string message){
    cout<<"server receive client message" <<message<<endl;
}

void onDisconnect(SocketClient *socket){
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

void freeMemory(){
	for(auto x : clientsVector){
		delete (std::string*) x->getTag();
		delete x;
	}
}

int startServer() {
	srand(time(NULL));

#ifdef USE_UNIX_DOMAIN
    //use unix domain socket
	SocketServer server;
    cout<<"server use unix domain socket " << UNIX_DOMAIN_SOCKET_NAME<<endl;
#else
    //use inter domain socket
	SocketServer server(SOCKET_PORT);
    cout<<"server use inter domain socket "<< SOCKET_IP_ADDR << " : " << SOCKET_PORT << endl;
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
		cout << "fail to bigdata start server" << endl;
	}

    //in most case, will not enter.
	freeMemory();
    return 0;
}

int stopServer()//TODO
{
    return 0;
}

#ifdef DebugMain
int main(int argc , char *argv[]){
    thread t = thread([](){
        startServer();
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
    t.join();

	return 0;
}
#endif

