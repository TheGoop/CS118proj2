CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=123456789
CLASSES=

default: clean server client

all: server client

server: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp
	$(CXX) -o dumClient $^ $(CXXFLAGS) dummyclient.cpp

client: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz *.data

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .
<<<<<<< HEAD

good: goodserver goodclient

goodserver: 
	gcc -o server good_server.c -I.

goodclient: 
	gcc -o client good_client.c -I. 
=======
	
>>>>>>> cdf6ba89d5481a01040fb9b795c9a449e375425c
