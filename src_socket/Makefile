SOURCE =weightServer.cpp sql.c lib/*.cpp utils.cpp curlPost.cpp
CXXFLAGS=-O2 -Wall -Wno-strict-aliasing -Wno-unused-variable
LDFLAGS=-L/usr/lib64/mysql -lmysqlclient -lglog -lpthread -lcrypt
INCPATHS=-I/usr/include/mysql -I ./ -Ilib/ -Icommon/

LDFLAGS_JSON_CURL += -lcurl -DENABLE_JSON_POST

LDFLAGS+=$(LDFLAGS_JSON_CURL) 
EXE = ./weightServer

$(EXE):$(SOURCE)
	g++ -std=c++11 -g -o $(EXE) $(SOURCE) $(LDFLAGS) $(INCPATHS) 
clean:
	rm -rf weightServer

#./server
#./client
