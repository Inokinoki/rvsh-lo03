all: rvsh RVSHServer
	rm *.o	

rvsh: main.o Command.o Session.o
	g++ main.o Command.o Session.o -lreadline -o rvsh

RVSHServer: Server_main.o Server.o
	g++ Server_main.o Server.o -lreadline -lpthread -o RVSHServer

Server_main.o: Server_main.cpp
	g++ -c Server_main.cpp -o Server_main.o

main.o: Session.h Command.h main.cpp
	g++ -c main.cpp -o main.o

Command.o: Session.h Command.h Command.cpp
	g++ -c Command.cpp -o Command.o

Session.o: Session.h Command.h Session.cpp
	g++ -c Session.cpp -o Session.o

clean:
	rm *.o



.PHONY: clean

