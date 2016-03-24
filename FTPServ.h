#ifndef _FTP_SERVER_H
#define _FTP_SERVER_H

#include "FTP.h"
#include <string>
#include <vector>
#include <pthread.h>
#include <sys/socket.h>


using std::string;
using std::vector;

extern const int DEFAULT_THREADNUM;					//默认的线程数量
extern const int INIT_MAX_CLI;						//存放线程号的数组大小
extern const int LISTENQUEUE;						//监听套接字队列长度
extern int threadnum;							//实际的线程数量

extern const char USERNAME[];						//用户名
extern const char PASSWD[];							//密码


extern char servpath[MAXLINE];						//服务器端路径
extern vector<string> servfilevec;						//服务器端当前路径下的文件

extern int in,out;								//用于存取记录位置
extern vector<int> clientfd;						//存放连接的客户端套接字
extern pthread_t* threadptr;	
extern pthread_mutex_t num_mutex;						//用于同步访问线程数量的锁
extern pthread_mutex_t client_mutex;					//用于同步访问clientfd的锁和条件变量
//extern pthread_mutex_t path_mutex;
extern pthread_mutex_t filevec_mutex;
extern pthread_cond_t client_cond;

void setFileVec(char*);
void initDataSock(int*,struct sockaddr_in*);
void passFileVec(int);
int splitFiles(char* data,string& filename,int flag);
void sendFiles(int,const string&,int);
void doSend(int,const string&);
void recvFiles(int,const string&,int);
void doRecv(int,const string&,unsigned long);
bool existFile(const string& file);

void* threadFunc(void*);
void communicateWithClient(int);
void sigFunc(int);
void sigThreadfunc(int);

#endif
