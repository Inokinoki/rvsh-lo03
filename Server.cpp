#include "Server.h"
#include <ctype.h>
#include <sys/socket.h> 

#define T_ACK 'A'
#define T_ENQ 'M'

void Host::save(ofstream& file){
    file<<hid<<" "<<hostname<<endl;
}

void Server::clearUpdateSession(){
    time_t t = time(NULL);
    for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++ ){
        // 30*60=1800 metier heure
        if(t-(*i)->getTime()>60*30){
            delete *i;
            session_list.erase(i);
        }
    }
}

void Session::sendMessage(char* message){
    if(message!=NULL){
        string temp(message);
        this->message_queue.push(temp);
    }
}

void Session::showMessage(ostream& f){
    while(!message_queue.empty()){
        f<<message_queue.front()<<endl;
        message_queue.pop();
    }
}

void Session::adminShowMessage(ostream& f){
    queue<string> temp_queue(message_queue);
    while(!temp_queue.empty()){
        f<<temp_queue.front()<<endl;
        temp_queue.pop();
    }
}

int User::login(char* p){
    if(!strcmp(p, password)){
        return 0;
    }
    else
        return 1;
}

void User::add_permission(int hid){
    bool e = false;
    for(list<int>::const_iterator i = permission_server.begin(); i!=permission_server.end(); i++){
        if(*i==hid){
            e = true;
            break;
        }
    }
    if(!e)
        permission_server.push_back(hid);
}

bool User::valide_permission(int hid){
    for(list<int>::const_iterator i = permission_server.begin(); i!=permission_server.end(); i++){
        if(*i==hid)
            return true;
    }
    return false;
}

void User::remove_permission(int hid){
    permission_server.remove(hid);
}

void User::change_password(char* n_password){
    strcpy(password, n_password);
}

void User::save(ofstream& file){
    file<<uid<<" "<<username<<" "<<password<<" ";
    for(list<int>::const_iterator i = permission_server.begin(); i!=permission_server.end(); i++){
        file<<*i<<" ";
    }
    file<<"-1"<<endl;
}

void Server::init(){
    ifstream ulist("~/.rvsh/userlist");
    int id, num_p;
    int permission[1024];
    char u[129], p[129];
    if(ulist){
        while(!ulist.eof()){
            ulist>>id;
            if(id==-1||ulist.eof())
                break;
            ulist>>u>>p;
            int temp_p;
            num_p=0;
            ulist>>temp_p;
            while(temp_p!=-1){
                permission[num_p++] = temp_p;
                ulist>>temp_p;
            }
            User* temp = new User(u, p, id, permission, num_p);
            user_list.push_back(temp);
        }
        ulist.close();
        
        cout<<"User list:"<<endl;
        for(list<User*>::const_iterator i = user_list.begin(); i!=user_list.end(); i++){
            cout<<"\t- "<<(*i)->getUsername()<<endl;
        }
    }
    ifstream hlist("~/.rvsh/hostlist");
    int hid;
    char name[129];
    if(hlist){
        while(!hlist.eof()){
            hlist>>hid;
            if(hid==-1||hlist.eof())
                break;
            hlist>>name;
            Host* temp = new Host(hid, name);
            host_list.push_back(temp);
        }
        hlist.close();
        cout<<"Host list:"<<endl;
        for(list<Host*>::const_iterator i = host_list.begin(); i!=host_list.end(); i++){
            cout<<"\t"<<(*i)->getHID()<<". "<<(*i)->getHostname()<<endl;
        }
    }
}

Server::~Server(){
    ofstream ulist("userlist");
    for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
        (*i)->save(ulist);
        delete *i;
    }
    ulist<<"-1";
    ulist.close();

    ofstream hlist("hostlist");
    for(list<Host*>::iterator i = host_list.begin(); i!=host_list.end(); i++){
        (*i)->save(hlist);
        delete *i;
    }
    hlist<<"-1";
    hlist.close();

    for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
        delete *i;
    }
    host_list.clear();
    user_list.clear();
    session_list.clear();
}

int Server::exec(int sid, char* command, stringstream& f, int connect_fd){
    if(command==NULL)
        return -1;
    Session* cSession = NULL;
    this->clearUpdateSession();
    if(sid!=-1){    // Not for login
        if(sid!=0){     // Normal
            for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                if((*i)->getSID()==sid){
                    cSession = *i;
                    if(cSession->getUser()==NULL)
                        sid=0;
                    break;
                }
            }
            // No such session
            if(cSession==NULL){
                f<<"Session '"<<sid<<"' n'existe pas!"<<endl;
                return -1;
            }
            // Show message
            cSession->showMessage(f);
        } else {        // Admin
            for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                if((*i)->avoirMessage()){
                    f<<"Session "<<(*i)->getSID()<<endl;
                    (*i)->adminShowMessage(f);
                }
            }
        }
    }
    
    // Parse arguments of command
    char** argv = new char*[10];
    int argc=1;
    argv[0] = new char[32];
    int argLength = 0;
    while(*command!=' '&&*command!='\0'){
        argv[0][argLength++] = *command;
        command++;
    }
    argv[0][argLength] = '\0';
    char* cursor = command;
    while(*cursor!='\0'){
        if(*cursor==' '){
            if(*(cursor+1)!=' '){
                if(argc>=10)   // Too much arg, impossible
                    break;
                else{    // Create a new arg
                    argv[argc++] = new char[32];
                    argLength = 0;
                }
            }
        }
        else{
            argv[argc-1][argLength] = *cursor;
            argLength++;
            if(*(cursor+1)==' ' || *(cursor+1)=='\0')
                argv[argc-1][argLength] = '\0';
        }
        cursor++;
    }
    if(!strcmp(argv[0], "connect")){
        if(argc>3){
            // Host
            Host* connect_host = NULL;
            for(list<Host*>::iterator i = host_list.begin(); i!=host_list.end(); i++){
                if(!strcmp((*i)->getHostname(), argv[1])){
                    // Host existe.
                    connect_host = *i;
                }
            }
            if(connect_host==NULL){
                f<<-1<<" Host '"<<argv[1]<<"' n'existe pas."<<endl;
                return -1;
            }
            // User
            User* connect_user = NULL;
            for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
                if(!strcmp((*i)->getUsername(), argv[2])){
                    // User existe.
                    connect_user = *i;
                }
            }
            if(connect_user==NULL){
                f<<-1<<" User '"<<argv[1]<<"' n'existe pas."<<endl;
                return -1;
            }
            // Valide permission
            if(connect_user->valide_permission(connect_host->getHID())){
                if(!connect_user->login(argv[3])){
                    int connect_sid=0;
                    for(list<Session*>::iterator i=session_list.begin();i!=session_list.end();i++){
                        if((*i)->getSID()>connect_sid){
                            connect_sid=(*i)->getSID();
                        }
                    }
                    connect_sid++;
                    Session* connect_session = 
                        new Session(connect_user, connect_host, connect_sid);
                    session_list.push_back(connect_session);
                    if(sid!=0)
                        cout<<"[INFO]"<<"User '"<<argv[2]<<"' connecte Host '"<<argv[1]<<"'."<<endl;
                    f<<connect_sid<<" User '"<<argv[2]<<"' sur Host '"<<argv[1]<<"'."<<endl;
                    return 0;
                } else {
                    f<<-1<<" User '"<<argv[2]<<"' cle incorrect."<<endl;
                    return -1;
                }
            } else {
                f<<-1<<" User '"<<argv[2]<<"' n'est pas accessible de Host '"<<argv[1]<<"'."<<endl;
                return -1;
            }
        }else if(argc>1){
            ifstream ifile("passwd");
            char admin_password[129]="123456";
            if(ifile){
                ifile>>admin_password;
                ifile.close();
            }
            if(!strcmp(admin_password, argv[1])){
                int connect_sid=0;
                for(list<Session*>::iterator i=session_list.begin();i!=session_list.end();i++){
                    if((*i)->getSID()>connect_sid){
                        connect_sid=(*i)->getSID();
                    }
                }
                connect_sid++;
                Session* connect_session = 
                    new Session(NULL, NULL, connect_sid);
                session_list.push_back(connect_session);
                f<<connect_sid<<endl;
                return 0;
            } else {
                f<<-1<<" Cle incorrect.";
                return -1;
            }
        }else {
            f<<-1<<" Usage:"<<endl;
            f<<"\tConnect hostname username password."<<endl;
            return -1;
        }
    }
    
    // execute
    if(sid!=-1){
        if(!strcmp(argv[0], "write")){
            if(argc>3){
                // Find
                User* toUser = NULL;
                Host* toHost = NULL;
                for(list<User*>::iterator i = user_list.begin(); i!=user_list.end();i++){
                    if(!strcmp((*i)->getUsername(), argv[1])){
                        toUser = *i;
                        break;
                    }
                }
                for(list<Host*>::iterator i = host_list.begin(); i!=host_list.end();i++){
                    if(!strcmp((*i)->getHostname(), argv[2])){
                        toHost = *i;
                        break;
                    }
                }
                if(toUser==NULL){
                    f<<"No User '"<<argv[1]<<"'"<<endl;
                    return -1;
                }
                if(toHost==NULL){
                    f<<"No Host '"<<argv[2]<<"'"<<endl;
                    return -1;
                }
                for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                    if((*i)->getUser()==toUser&&(*i)->getHost()==toHost){
                        char temp_message[512]="";
                        strcat(temp_message, "Message from ");
                        if(sid>0){
                            strcat(temp_message, cSession->getUser()->getUsername());
                            strcat(temp_message, "@");
                            strcat(temp_message, cSession->getHost()->getHostname());
                        } else {
                            strcat(temp_message, "admin");
                        }
                        (*i)->sendMessage(temp_message);
                        (*i)->sendMessage(argv[3]);
                    }
                }
            } else {
                f<<"Usage:"<<endl;
                f<<"\twrite username hostname message"<<endl;
                return -1;
            }
            return 0;
        }
        if(!strcmp(argv[0], "who")){
            if(sid!=0){
                // Find Session
                if(cSession==NULL){
                    return -1;
                }
                // Get host
                Host* who_host = cSession->getHost();
                // List all
                for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                    if((*i)->getHost()==who_host){
                        time_t rawtime = (*i)->getTime();
                        struct tm* timeinfo = localtime(&rawtime);
                        f<<(*i)->getUser()->getUsername()<<" \t"<<
                            asctime(timeinfo);
                    }
                }
                return 0;
            } else {
                f<<"Dans reseau virtuel : "<<endl;
                for(list<Session*>::const_iterator i = session_list.begin(); i!=session_list.end(); i++){
                    Session* rusers_session = *i;
                    if(rusers_session->getUser()==NULL)
                        continue;
                    time_t rawtime = rusers_session->getTime();
                    struct tm* timeinfo = localtime(&rawtime);
                    f<<rusers_session->getUser()->getUsername()<<"@"<<
                        rusers_session->getHost()->getHostname()<<" \t"<<
                        asctime(timeinfo);
                }
            }
            return 0;
        }
        if(!strcmp(argv[0], "finger")){
            if(sid!=0){
                // Find Session
                Session* finger_session = NULL;
                for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                    if((*i)->getSID()==sid){
                        finger_session = *i;
                        break;
                    }
                }
                if(finger_session==NULL){
                    f<<"Session '"<<sid<<"' n'existe pas!"<<endl;
                    return -1;
                }
                // Get user
                User* finger_user = finger_session->getUser();
                // List all
                f<<"Login\tName\tHost\tIdle\tLogin Time\t\tOffice\tPhone"<<endl;
                for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                    if((*i)->getUser()==finger_user){
                        time_t rawtime = (*i)->getTime();
                        struct tm* timeinfo = localtime(&rawtime);
                        f<<(*i)->getUser()->getUsername()<<"\t"<<
                            (*i)->getUser()->getUsername()<<"\t"<<
                            (*i)->getHost()->getHostname()<<"\t\t"<<
                            asctime(timeinfo);
                    }
                }
            } else {
                f<<"Login\tName\tHost\tIdle\tLogin Time\t\tOffice\tPhone"<<endl;
                for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                    if((*i)->getUser()==NULL)
                        continue;
                    time_t rawtime = (*i)->getTime();
                    struct tm* timeinfo = localtime(&rawtime);
                    f<<(*i)->getUser()->getUsername()<<"\t"<<
                        (*i)->getUser()->getUsername()<<"\t"<<
                        (*i)->getHost()->getHostname()<<"\t\t"<<
                        asctime(timeinfo);
                }
            }
            return 0;
        }
        if(!strcmp(argv[0], "rhost")){
            f<<"Dans reseau virtuel : "<<endl;
            for(list<Host*>::const_iterator i = host_list.begin(); i!=host_list.end(); i++){
                f<<"\t-"<<(*i)->getHostname()<<endl;
            }
            return 0;
        }
        if(!strcmp(argv[0], "rusers")){
            f<<"Dans reseau virtuel : "<<endl;
            for(list<Session*>::const_iterator i = session_list.begin(); i!=session_list.end(); i++){
                Session* rusers_session = *i;
                if(rusers_session->getUser()==NULL)
                    continue;
                time_t rawtime = rusers_session->getTime();
                struct tm* timeinfo = localtime(&rawtime);
                f<<rusers_session->getUser()->getUsername()<<"@"<<
                    rusers_session->getHost()->getHostname()<<" \t"<<
                    asctime(timeinfo);
            }
            return 0;
        }
        if(!strcmp(argv[0], "passwd")){
            if(sid!=0){
                for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                    if((*i)->getSID()==sid){
                        User* passwd_user = NULL;
                        passwd_user = (*i)->getUser();
                        f<<"Enter password:";
                        char temp_password[129];
                        if(connect_fd==-1){
                            // Console mode
                            cout<<f.str();
                            f.str("");
                            cin>>temp_password;
                        } else {
                            char t_sig[1];
                            t_sig[0]=T_ENQ;
                            if(send(connect_fd, t_sig, 1, 0)!=-1){
                                const string b = f.str();
                                f.str("");
                                if(send(connect_fd, b.c_str(), strlen(b.c_str()),0) == -1)  
                                    cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
                            } else {
                                cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
                            }
                            int password_length
                                 = recv(connect_fd, temp_password, 128, 0);
                            if(!isalpha(temp_password[0])&&!isdigit(temp_password[0])){
                                f<<"Format de mot de passe incorrect."<<endl;
                                return -1;
                            }
                            temp_password[password_length-1]='\0';
                        }
                        passwd_user->change_password(temp_password);
                        f<<"Password changed."<<endl;
                        return 0;
                    }
                }
                f<<"Session '"<<sid<<"' n'existe pas!"<<endl;
                return -1;
            } else {
                f<<"Enter password:";
                char temp_password[129];
                if(connect_fd==-1){
                    // Console mode
                    cout<<f.str();
                    f.str("");
                    cin>>temp_password;
                }else {
                    char t_sig[1];
                    t_sig[0]=T_ENQ;
                    if(send(connect_fd, t_sig, 1, 0)!=-1){
                        const string b = f.str();
                        f.str("");
                        if(send(connect_fd, b.c_str(), strlen(b.c_str()),0) == -1)  
                            cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
                    } else {
                        cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
                    }
                    int password_length
                            = recv(connect_fd, temp_password, 128, 0);
                    if(!isalpha(temp_password[0])&&!isdigit(temp_password[0])){
                        f<<"Format de mot de passe incorrect."<<endl;
                        return -1;
                    }
                    temp_password[password_length-1]='\0';
                }
                ofstream ofile("passwd");
                ofile<<temp_password;
                ofile.close();
                f<<"Password changed."<<endl;
                return 0;
            }
        }
        
        if(sid==0){
            if(!strcmp(argv[0], "host")){
                if(argc>2){
                    if(!strcmp(argv[1], "-a")){
                        int max_hid=0;
                        for(list<Host*>::iterator i = host_list.begin(); i!=host_list.end(); i++){
                            // max_hid+1
                            if((*i)->getHID()>max_hid)
                                max_hid=(*i)->getHID();
                            // Already have a host named argv[2]?
                            if(!strcmp((*i)->getHostname(), argv[2])){
                                f<<"Deja existe un Host '"<<argv[2]<<"' !"<<endl;
                                return -1;
                            }
                        }
                        max_hid++;
                        // Add to list
                        Host* temp_host = new Host(max_hid, argv[2]);
                        host_list.push_back(temp_host);
                        f<<"Host '"<<argv[2]<<"' ajoute!"<<endl;
                        return 0;
                    }else if(!strcmp(argv[1], "-r")){
                        // Find and delete
                        for(list<Host*>::iterator i = host_list.begin(); i!=host_list.end(); i++){
                            if(!strcmp((*i)->getHostname(), argv[2])){
                                delete *i;
                                host_list.erase(i);
                                f<<"Host '"<<argv[2]<<"' supprime!"<<endl;
                                return 0;
                                break;
                            }
                        }
                        f<<"Host '"<<argv[2]<<"'n'existe pas!"<<endl;
                        return -1;
                    }
                } else {
                    f<<"Usage:"<<endl;
                    f<<"\thost -a hostname\t\t Add a host."<<endl;
                    f<<"\thost -r hostname\t\t Remove a host."<<endl;
                    return -1;
                }
            }
            if(!strcmp(argv[0], "users")){
                cout<<argc<<endl;
                if(argc>3){
                    if(!strcmp(argv[1], "-ap")){
                        // Find and add
                        User* addp_users = NULL;
                        for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
                            if(!strcmp((*i)->getUsername(), argv[2])){
                                addp_users = *i;
                            }
                        }
                        if(addp_users==NULL){
                            f<<"User '"<<argv[2]<<"'n'existe pas!"<<endl;
                            return -1;
                        }
                        for(int j=3;j<argc;j++){
                            Host* addp_host = NULL;
                            for(list<Host*>::iterator i = host_list.begin(); i!=host_list.end(); i++){
                                if(!strcmp((*i)->getHostname(), argv[j])){
                                    addp_host = *i;
                                    addp_users->add_permission((*i)->getHID());
                                    f<<"Ajoute permission de'"<<argv[2]<<"' sur '"<<(*i)->getHostname()<<"'."<<endl;
                                    break;
                                }
                            }
                            if(addp_host==NULL)
                                f<<"Host '"<<argv[j]<<"' n'existe pas."<<endl;
                        }
                        return 0;
                    } else if(!strcmp(argv[1], "-rp")){
                        // Find and delete
                        User* removep_users = NULL;
                        for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
                            if(!strcmp((*i)->getUsername(), argv[2])){
                                removep_users = *i;
                            }
                        }
                        if(removep_users==NULL){
                            f<<"User '"<<argv[2]<<"'n'existe pas!"<<endl;
                            return -1;
                        }
                        for(int j=3;j<argc;j++){
                            Host* removep_host = NULL;
                            for(list<Host*>::iterator i = host_list.begin(); i!=host_list.end(); i++){
                                if(!strcmp((*i)->getHostname(), argv[j])){
                                    removep_host = *i;
                                    f<<"Supprime permission de'"<<argv[2]<<"' sur '"<<(*i)->getHostname()<<"'."<<endl;
                                    removep_users->remove_permission((*i)->getHID());
                                    break;
                                }
                            }
                            if(removep_host==NULL)
                                f<<"Host '"<<argv[j]<<"' n'existe pas."<<endl;
                        }
                        return 0;
                    } else if(!strcmp(argv[1], "-m")){
                        // Find and modifier
                        for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
                            if(!strcmp((*i)->getUsername(), argv[2])){
                                (*i)->change_password(argv[3]);
                                f<<"User '"<<argv[2]<<"' password changed!"<<endl;
                                return 0;
                            }
                        }
                        f<<"User '"<<argv[2]<<"'n'existe pas!"<<endl;
                        return -1;
                    } else {
                        f<<"Usage:"<<endl;
                        f<<"\tusers -a username\t\t Add a user."<<endl;
                        f<<"\tusers -r username\t\t Remove a user."<<endl;
                        f<<"\tusers -ap username host1 host2 ...\t\t Add a user to host."<<endl;
                        f<<"\tusers -rp username host1 host2 ...\t\t Remove a user from host."<<endl;
                        f<<"\tusers -m username password\t\t Modify password of a user."<<endl;
                        return -1;
                    }
                } else if(argc>2){
                    if(!strcmp(argv[1], "-a")){
                        int max_uid=0;
                        for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
                            // max_uid+1
                            if((*i)->getUID()>max_uid)
                                max_uid=(*i)->getUID();
                            // Already have a host named argv[2]?
                            if(!strcmp((*i)->getUsername(), argv[2])){
                                f<<"Deja existe un User '"<<argv[2]<<"' !"<<endl;
                                return -1;
                            }
                        }
                        max_uid++;
                        f<<"Enter password:";
                        char temp_password[129];
                        if(connect_fd==-1){
                            // Console mode
                            cout<<f.str();
                            f.str("");
                            cin>>temp_password;
                        } else {
                            char t_sig[1];
                            t_sig[0]=T_ENQ;
                            if(send(connect_fd, t_sig, 1, 0)!=-1){
                                const string b = f.str();
                                f.str("");
                                if(send(connect_fd, b.c_str(), strlen(b.c_str()),0) == -1)  
                                    cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
                            } else {
                                cout<<"[Error]Envoyer a connection "<<connect_fd<<" eche."<<endl;
                            }
                            int password_length
                                 = recv(connect_fd, temp_password, 128, 0);
                            if(!isalpha(temp_password[0])&&!isdigit(temp_password[0])){
                                f<<"Format de mot de passe incorrect."<<endl;
                                return -1;
                            }
                            temp_password[password_length-1]='\0';
                        }
                        // Add to list
                        User* temp_user = new User(argv[2], temp_password, max_uid, NULL, 0);
                        user_list.push_back(temp_user);
                        f<<"User '"<<argv[2]<<"' ajoute!"<<endl;
                        return 0;
                    } else if(!strcmp(argv[1], "-r")){
                        // Find and delete
                        for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
                            if(!strcmp((*i)->getUsername(), argv[2])){
                                delete *i;
                                user_list.erase(i);
                                f<<"User '"<<argv[2]<<"' supprime!"<<endl;
                                return 0;
                            }
                        }
                        f<<"User '"<<argv[2]<<"'n'existe pas!"<<endl;
                        return -1;
                    }  else {
                        f<<"Usage:"<<endl;
                        f<<"\tusers -a username\t\t Add a user."<<endl;
                        f<<"\tusers -r username\t\t Remove a user."<<endl;
                        f<<"\tusers -ap username host1 host2 ...\t\t Add a user to host."<<endl;
                        f<<"\tusers -rp username host1 host2 ...\t\t Remove a user from host."<<endl;
                        f<<"\tusers -m username password\t\t Modify password of a user."<<endl;
                        return -1;
                    }
                } else {
                    f<<"Usage:"<<endl;
                    f<<"\tusers -a username\t\t Add a user."<<endl;
                    f<<"\tusers -r username\t\t Remove a user."<<endl;
                    f<<"\tusers -ap username host1 host2 ...\t\t Add a user to host."<<endl;
                    f<<"\tusers -rp username host1 host2 ...\t\t Remove a user from host."<<endl;
                    f<<"\tusers -m username password\t\t Modify password of a user."<<endl;
                    return -1;
                }
            }
            if(!strcmp(argv[0], "afinger")){
                if(argc>1){
                    // Get user
                    User* afinger_user = NULL;
                    for(list<User*>::iterator i = user_list.begin(); i!=user_list.end(); i++){
                        if(!strcmp((*i)->getUsername(), argv[1])){
                            afinger_user = *i;
                            break;
                        }
                    }
                    if(afinger_user==NULL){
                        f<<"User '"<<argv[1]<<"' n'existe pas."<<endl;
                        return -1;
                    }
                    // List all
                    f<<"Login: "<<afinger_user->getUsername()<<"\t\tName: "<<afinger_user->getUsername()<<endl;
                    f<<"Directory: /\t\tShell: rvsh"<<endl;
                    for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                        if((*i)->getUser()==afinger_user){
                            time_t rawtime = (*i)->getTime();
                            struct tm* timeinfo = localtime(&rawtime);
                            f<<"On since "<<asctime(timeinfo);
                        }
                    }
                    return 0;
                } else {
                    f<<"Usage:"<<endl;
                    f<<"\tafinger username"<<endl;
                    return -1;
                }
            }
        }

        if(!strcmp(argv[0], "exit")){
            if(sid==0&&cSession==NULL){
                f<<"All connection will be disconnected."<<endl;
                for(list<Session*>::iterator i = session_list.begin(); i!=session_list.end(); i++){
                    f<<"Disconnect Session "<<(*i)->getSID()<<endl;
                }
                return 0;
            } else {
                if(cSession==NULL){
                    f<<"1";
                    return -1;
                }
                session_list.remove(cSession);
                f<<"0";
                return 0;
            }
            return 0;
        }
    }
    f<<"No such command: "<<argv[0]<<endl;
    return -1;
}
