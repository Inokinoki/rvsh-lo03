#include "Command.h"
#include <iostream>
#include <stdlib.h>
#include <string>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h> 
#include <string.h>  
#include <errno.h>  
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sstream>

using namespace std;

void Session::setPassword(){
	char temp[129];
	std::cout << "Mot de passe:";
	std::cin >> temp;
	strcpy(password, temp);
}

void Session::setUsername(char* un){
	strcpy(username, un);
}

void Session::setHostname(char* hn){
	strcpy(hostname, hn);
}

int Session::send(const char* sendline){
	int sockfd, n,rec_len; 
	char buf[4096];
	struct sockaddr_in servaddr;  

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){  
		cout<<"[ERROR]Create socket error: "<<strerror(errno)<<" "<<errno<<endl;  
		return -1; 
	}  
	memset(&servaddr, 0, sizeof(servaddr));  
	servaddr.sin_family = AF_INET;  
	servaddr.sin_port = htons(12500);  
	if(inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0){  
		cout<<"[ERROR]Inet_pton error for localhost"<<endl;
		return -2;  
	}  

	if( ::connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){  
		cout<<"[ERROR]Connect error: "<<strerror(errno)<<" "<<errno<<endl;   
		return -3; 
	} 
	
	//cout<<sendline<<endl;

	// Non 'Enter'
	if( ::send(sockfd, sendline, strlen(sendline)+1, 0) < 0)  {  
		cout<<"[ERROR]Send msg error: "<<strerror(errno)<<" "<<errno;  
		return -4;
	}
	char t_sig;
	rec_len = recv(sockfd, buf, 4096,0);
	t_sig=buf[0];
	char* command;
	if(rec_len==1){
		do{	// Attendre pour message utile
			rec_len = recv(sockfd, buf, 4096,0);
		}while(rec_len==0);
		command = buf;
	} else {
		command = buf+1;
	}
	if(isdigit(command[0])){
		// Connect mode
		int sid = 0;
		while(*command>='0'&&*command<='9'){
			switch(*command){
				case '0':sid=sid*10;break;
				case '1':sid=sid*10+1;break;
				case '2':sid=sid*10+2;break;
				case '3':sid=sid*10+3;break;
				case '4':sid=sid*10+4;break;
				case '5':sid=sid*10+5;break;
				case '6':sid=sid*10+6;break;
				case '7':sid=sid*10+7;break;
				case '8':sid=sid*10+8;break;
				case '9':sid=sid*10+9;break;
			}
			command++;
		}
		this->sid = sid;
	} else if(command[0]=='-') {
		while(*command!=' '){
			command++;
		}
	}
	buf[rec_len] = '\0';
	cout<<command<<endl;  
	if(t_sig=='M'){
		char temp[512];
		cin>>temp;
		if( ::send(sockfd, temp, strlen(temp)+1, 0) < 0){  
			cout<<"[ERROR]Send msg error: "<<strerror(errno)<<" "<<errno;  
			return -4;
		}
		rec_len = recv(sockfd, buf, 4096,0);
		t_sig=buf[0];
		if(rec_len==1){
			do{
				// Attendre pour message utile
				rec_len = recv(sockfd, buf, 4096,0);
			}while(rec_len==0);
			command = buf;
		} else {
			command = buf+1;
		}
		buf[rec_len] = '\0';
		cout<<command<<endl;
	}
	close(sockfd);
	return 0;
}

int Session::connect(){  
	stringstream temp;
	temp<<"connect ";
	if(runlevel==1){
		temp<<password;
	} else {
		temp<<hostname<<" "<<username<<" "<<password;
	}
	this->send(temp.str().c_str());
	
	if(sid<0)
		return 1;
	return 0;
}

void Session::attendrePourCommande() {
	char* commande;
	char* prompt;
	if (runlevel == 0) {
		prompt = new char[128+1+128+1+1];
		//username+@+hostname+>+\0
		strcpy(prompt, username);
		strcat(prompt, "@");
		strcat(prompt, hostname);
		strcat(prompt, ">");
	}
	else if (runlevel == 1) {
		prompt = new char[6];
		strcpy(prompt, "rvsh>");
	}
	else {
		std::cout << "Mode error. Session terminated." << std::endl;
		return;
	}
	system("clear");
	do{
		commande = readline(prompt);
		add_history(commande);
		
		if(!strcmp(commande, "exit")){
			stringstream temp;
			temp<<sid<<" "<<"exit";
			if(send(temp.str().c_str())!=0){
				cout<<"Error"<<endl;
			}
			break;
		}
		else{
			RVSHCommand* c = new RVSHCommand(commande, this);
			if(c->exec()!=0){
				cout<<"Error"<<endl;
			}
			delete c;
		}
	} while (true);
	delete prompt;
}

Session::Session(int level):runlevel(level), sid(-1){
	memset(username, 0, 128 * sizeof(char));
	memset(hostname, 0, 128 * sizeof(char));
	memset(password, 0, 128 * sizeof(char));
}


Session::~Session(){
	//std::cout << username << std::endl;
	//std::cout << hostname << std::endl;
	//std::cout << password << std::endl;
}
