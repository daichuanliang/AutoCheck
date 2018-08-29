SOURCE = main.cpp sql.c
CXXFLAGS = -std=c++11 -Wall -Wno-strict-aliasing -Wno-unused-variable
LDFLAGS = -L/usr/lib64/mysql -lmysqlclient -lglog -lpthread 
INCPATHS = -I/usr/include/mysql


EXE = ./main

$(EXE):$(SOURCE)
	g++ -g -o $(EXE) $(SOURCE) $(LDFLAGS) $(INCPATHS) 
clean:
	rm -rf main
