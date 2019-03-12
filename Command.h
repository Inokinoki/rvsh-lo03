#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <string.h>
#include <iostream>
#include "Session.h"

class Command{
protected:
	int argc;
	int max;
	char** argv;
	Session* session;
	int sid;
public:
	Command(char* param, Session* s):argc(1), max(10), 
			session(s),sid(s->getSID()){
		argv = new char*[max];
		argv[0] = new char[32];
		int argLength = 0;
		while(*param!=' '&&*param!='\0'){
            argv[0][argLength++] = *param;
            param++;
		}
		argv[0][argLength] = '\0';
		char* cursor = param;
		while(*cursor!='\0'){
			if(*cursor==' '){
				if(*(cursor+1)!=' '){
					if(argc>=max)   // Too much arg, impossible
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
	}

	virtual int exec() { return 0; }
	virtual ~Command(){
		for(int i=0;i<argc;i++)
			delete argv[i];
		delete argv;
		session = 0;
	}
};

class RVSHCommand : public Command{
protected:
public:
	RVSHCommand(char* param, Session* session)
		: Command(param, session){
	}
	int exec();
};

#endif
