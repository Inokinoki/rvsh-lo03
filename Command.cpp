#include "Command.h"
#include <string.h>
#include <sstream>
#include "Session.h"

// Not login -- 4
// No such command -- 3
// Permission denied -- 2
// Runtime error -- 1
// Run ok -- 0
// Run failed -- -1

using namespace std;

int RVSHCommand::exec(){ 
	stringstream temp;
	temp<<sid<<" ";

	if(!strcmp(argv[0], "connect")){
		if(argc>=3){
			Session* session = new Session(0);
			session->setUsername(argv[2]);
			session->setHostname(argv[1]);
			std::cout << "Connect Mode: " << argv[2] << "@" << argv[1] << std::endl;
			session->setPassword();
			if (!session->connect()) {
				session->attendrePourCommande();
			}
			else {
				return -1;
			}
			delete session;
		} else {
			std::cout << "Usage:" << std::endl;
			std::cout << "\t connect hostname username" << std::endl;
		}
		return 0;
	}else if(!strcmp(argv[0], "su")){
		if(argc>=2){
			if(!strcmp(this->session->getHostname(), "")){
				std::cout<<"Admin, vous ne etes pas sur un host. su eche"<<endl;
				return -1;
			}
			Session* session = new Session(0);
			session->setUsername(argv[1]);
			session->setHostname(this->session->getHostname());
			std::cout << "Connect Mode: " << argv[1] << "@" << this->session->getHostname() << std::endl;
			session->setPassword();
			if (!session->connect()) {
				session->attendrePourCommande();
			}
			else {
				return -1;
			}
			delete session;
		} else {
			std::cout << "Admin Mode" << std::endl;
			Session* session = new Session(1);
			session->setPassword();
			if (!session->connect()) {
				session->attendrePourCommande();
			}
			else {
				return -1;
			}
			delete session;
		}
		return 0;
	}

	for(int i=0;i<argc;i++){
		temp<<argv[i];
		if(i!=argc-1){
			temp<<" ";
		}
	}
	
	session->send(temp.str().c_str());
	
	return 0;
}
