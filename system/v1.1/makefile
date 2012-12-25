all: PROXY RM TM IF CM TASK
PROXY: util.h lib.h Proxy.cpp
	g++ -O2 -Wall Proxy.cpp -g -o PROXY -lpthread -lrt
	cp PROXY bin/
	rm PROXY
RM: util.h lib.h ResourceManager.cpp
	g++ -O2 -Wall ResourceManager.cpp -g -o RM -lpthread
	cp RM bin/
	rm RM
TM: util.h lib.h TaskManager.cpp
	g++ -O2 -Wall TaskManager.cpp -g -o TM -lpthread
	cp TM bin/
	rm TM
IF: util.h lib.h Interface.cpp
	g++ -O2 -Wall Interface.cpp -g -o IF -lpthread
	cp IF bin/
	rm IF
CM: util.h lib.h Compiler.cpp
	g++ -O2 -Wall Compiler.cpp -g -o CM -lpthread
	cp CM bin/
	rm CM
TASK: util.h lib.h Task.cpp
	g++ -O2 -Wall Task.cpp -g -o TASK -lpthread
	cp TASK bin/
	rm TASK
clean:
	rm -f bin/PROXY bin/TM bin/RM bin/IF bin/CM bin/TASK
