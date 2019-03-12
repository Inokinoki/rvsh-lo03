#ifndef SESSION_H
#define SESSION_H

#include <string.h>

class Session{
private:
	char username[129];
	char hostname[129];
	char password[129];
	int sid;
	int runlevel;	// 0 -- connect 1 -- admin
public:
	void setPassword();
	void setUsername(char* un);
	void setHostname(char* hn);
	int connect();
	int send(const char* command);
	void attendrePourCommande();
	int getRunLevel(){ return runlevel; }
	char* getUsername(){ return username; }
	char* getHostname(){ return hostname; }
	int getSID() { return sid; }
	Session(int level);
	virtual ~Session();
};

#endif

