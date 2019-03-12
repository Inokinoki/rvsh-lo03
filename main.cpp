#include <iostream>
#include <string.h>
#include "Session.h"

using namespace std;

void printHelpMessage();

int main(int argc, char* argv[]) {
	if (argc > 1) {
		if (!strcmp(argv[1], "-admin")) {
			std::cout << "Admin Mode(123456 for default password)" << std::endl;
			Session* session = new Session(1);
			session->setPassword();
			if (!session->connect()) {
				session->attendrePourCommande();
			}
			delete session;
		}
		else if (!strcmp(argv[1], "-connect")) {
			if (argc >= 4) {
				Session* session = new Session(0);
				session->setUsername(argv[3]);
				session->setHostname(argv[2]);
				std::cout << "Connect Mode: " << argv[3] << "@" << argv[2] << std::endl;
				session->setPassword();
				if (!session->connect()) {
					session->attendrePourCommande();
				}
				delete session;
			}
			else {
				printHelpMessage();
				return 1;
			}
		}
		else {
			printHelpMessage();
			return 1;
		}
	}
	else {
		printHelpMessage();
		return 1;
	}
	return 0;
}

void printHelpMessage(){
	std::cout << "Usage:" << std::endl;
	std::cout << "\t rvsh -admin" << std::endl;
	std::cout << "\t\t Enter Admin Mode." << std::endl;
	std::cout << "\t rvsh -connect hostname username" << std::endl;
	std::cout << "\t\t Connect with 'hostname' as a user 'username'." << std::endl;
}
