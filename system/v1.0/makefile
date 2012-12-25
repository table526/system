all: PROXY RM
PROXY: util.h lib.h Proxy.cpp
	g++ -O2 -Wall Proxy.cpp -g -o PROXY
	cp PROXY bin/
	rm PROXY
RM: util.h lib.h ResourceManager.cpp
	g++ -O2 -Wall ResourceManager.cpp -g -o RM
	cp RM bin/
	rm RM
clean:
	rm -f bin/PROXY
