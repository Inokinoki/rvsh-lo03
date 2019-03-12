#include <iostream>
#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fstream>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>  
#include <errno.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <unistd.h>
#include <pthread.h>

#define MAXLINE 4096

#define T_ACK 'A'
#define T_ENQ 'M'

using namespace std;

Server* server;
int socket_fd;
int pid = 0;
bool over = false;

static void Server_exit(int signo)  {
    over = true;
    if ( socket_fd != -1) {  
          close( socket_fd );  
          socket_fd = -1;  
    }
    if(server != NULL){
        delete server;
    }
    exit(0);
} 

void* thread_main(void *str){
    char buff[4096];  
    int n;
    pthread_t pth;
    int connect_fd;
    if( (connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){  
        if(!over) {
            cout<<"[ERROR]Socket error: "<<strerror(errno)<<" "<<errno<<endl; 
            pthread_create(&pth, NULL, thread_main, (void *)(&pth));
        }
        return NULL;
    }
    pthread_create(&pth, NULL, thread_main, (void *)(pth));
    n = recv(connect_fd, buff, MAXLINE, 0);
    buff[n-1] = '\0';
    stringstream temp;
    temp.str("");
    char* command = buff;
    if(buff[0]>='0'&&buff[0]<='9'){
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
        if(sid!=0){
            while(*command==' ')
                command++;
            if(server->exec(sid, command, temp, connect_fd)){
                cout<<"[Error]Session "<<sid<<" exec '"<<command<<"' eche."<<endl;
            } else {
                cout<<"[INFO]"<<"Session "<<sid<<" :"<<command<<endl;
            }
        } else {
            temp<<"[Error]Invalid session."<<endl;
            cout<<"[Error]Session "<<sid<<" n'existe pas."<<endl;
        }
    } else {
        cout<<command<<endl;
        if(server->exec(-1, command, temp, connect_fd)){
            cout<<"[Error]Connect "<<connect_fd<<" exec '"<<buff<<"' eche."<<endl;
        }
    }
    char t_sig[1];
    t_sig[0]=T_ACK;
    if(send(connect_fd, t_sig, 1, 0)!=-1){
        const string b = temp.str();
        temp.str("");
        //cout<<b<<endl;
        if(send(connect_fd, b.c_str(), strlen(b.c_str()),0) == -1)  
            cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
    } else {
        cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
    }
    close(connect_fd);
    
    return NULL;
}

int main(){
    signal(SIGINT, Server_exit);

    pthread_t pth;
    struct sockaddr_in servaddr;

    server = new Server();
    char* command;
    //Init Socket  
    if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
        cout<<"[EMER]Create socket error: "<< strerror(errno)<<" "<<errno;
        delete server;  
        exit(0);  
    }
    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(12500); 
    if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
        cout<<"[EMER]Bind socket error: "<< strerror(errno)<<" "<<errno;
        delete server;    
        exit(0);  
    }  
    // Listen
    if( listen(socket_fd, 10) == -1){  
        cout<<"[EMER]Listen error: "<< strerror(errno)<<" "<<errno;  
        delete server;
        exit(0);  
    }

    // Open thread
    pthread_create(&pth, NULL, thread_main, (void *)(&pth));

    // Main loop
    do{
        command = readline("rvsh-console>");
        stringstream main_buffer;
        add_history(command);
        server->exec(0, command, main_buffer, -1);
        cout<<main_buffer.str()<<endl;
    } while(strcmp(command, "exit"));
    over = true;
    close(socket_fd);
    delete server;
    exit(0);
    return 0;
}
