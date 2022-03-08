CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=305414381_505370692_305340620_005376979
CLASSES=

all: server client

server: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp -lrt
	
client: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp -lrt

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz *.data

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .
	
