#ifndef _FTP_CLIENT_H
#define _FTP_CLIENT_H

#include "FTP.h"
#include <string>
#include <vector>
#define ClientCMDNUM 9
extern const char* ClientCMD[ClientCMDNUM];					//客户端的命令列表
extern char clientpath[MAXLINE];							//客户端的当前路径
extern std::vector<std::string> clientfilevec;					//客户端当前路径下的文件名列表

enum {User = 0,Pass,Pasv,Ls,Get,Put,Quit,Lcd,Cd};

void showFileVec(int);
int passPasvCmd(int,char*);
void downloadFiles(int);
void doDownload(int,const char*,unsigned long);
void uploadFiles(int,int);
void doUpload(int,const std::string&);
int connectDatasock(int,int);
bool splitFiles(char* data,std::string& filename,int& numtodeal);
bool existFile(const std::string& file);
bool changeLocalDir(std::string&);

#endif
