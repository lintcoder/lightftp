#ifndef _FTP_H
#define _FTP_H

#include <string>
#include <vector>

#define MAXLINE 50
#define MAXSIZE 1000
#define ServerCMDNUM 7
extern const int cmdport;			//服务器端的命令套接字端口

extern const char rightmsg[10];		
extern const char wrongmsg[10];
extern const char* ServerCMD[];

void err_sys(const char*);
unsigned long getFileSize(const std::string&);
void setFileVec(std::vector<std::string>&,char* path);
void printLocalFile(const std::vector<std::string>&);

#endif
