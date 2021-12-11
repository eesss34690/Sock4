CXX=g++

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system -lboost_filesystem
	SRC=/usr/bin
endif
ifeq ($(UNAME_S),Darwin)
	CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system -lboost_filesystem-mt
	SRC=/bin
endif

CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

all: socks_server console.cgi
socks_server: socks_server.cpp
	$(CXX) -o $@ $^ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

console.cgi: console.cpp
	$(CXX) -o $@ $^ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

precompile: ./cmds/noop.cpp ./cmds/number.cpp ./cmds/removetag.cpp ./cmds/removetag0.cpp ./cmds/delayedremovetag.cpp
	cp ${SRC}/ls ./bin
	cp ${SRC}/cat ./bin
	$(CXX) ./cmds/delayedremovetag.cpp -o ./bin/delayedremovetag
	$(CXX) ./cmds/noop.cpp -o ./bin/noop
	$(CXX) ./cmds/number.cpp -o ./bin/number
	$(CXX) ./cmds/removetag.cpp -o ./bin/removetag
	$(CXX) ./cmds/removetag0.cpp -o ./bin/removetag0

clean:
	rm -f socks_server console.cgi
