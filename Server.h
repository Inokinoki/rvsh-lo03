#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <queue>
#include <iostream>
#include <string.h>
#include <time.h>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

class Host{
private:
    int hid;
    char hostname[129];
public:
    Host(int id, char* h):hid(id){
        strcpy(hostname, h);
    }
    int getHID() const{ return hid; }
    const char* getHostname() const { return hostname; }
    void save(ofstream& file);
};

class User{
private:
    int uid;
    char username[129];
    char password[129];
    list<int> permission_server;     // hid
public:
    User(char* u, char* p, int id, int* permission, int num_p): uid(id){
        strcpy(username, u);
        strcpy(password, p);
        for(int i=0;i<num_p;i++)
            permission_server.push_back(permission[i]);
    }
    ~User(){
        permission_server.clear();
    }
    const int getUID() const { return uid; }
    const char* getUsername() const { return username; };
    int login(char* p);
    void add_permission(int hid);
    void remove_permission(int hid);
    bool valide_permission(int hid);
    void change_password(char* n_password);
    void save(ofstream& file);
};

class Session{
private:
    User* user;
    Host* host;
    time_t login_time;
    int sid;
    queue<std::string> message_queue;
public:
    Session(User* u, Host* h, int s):user(u), host(h), sid(s){
        time(&login_time);
    }
    bool avoirMessage(){ return !message_queue.empty(); }
    void adminShowMessage(ostream& f);
    void showMessage(ostream& f);
    void sendMessage(char* message);
    int getSID() const{ return sid; }
    time_t getTime() const{ return login_time; }
    User* getUser() const{ return user; }
    Host* getHost() const{ return host; }
};

class Server{
public:
    Server(){ init(); }
    int exec(int sid, char* command, stringstream& f, int connect_fd);
    ~Server();
private:
    void init();
    void clearUpdateSession();
    list<Host*> host_list;              // Added in exec() ou init()
    list<User*> user_list;              // Added in exec() ou init()
    list<Session*> session_list;    // Added in exec()
};

#endif // SERVER_H
