#include "FTPServ.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

using std::string;

const int DEFAULT_THREADNUM = 5;
const int INIT_MAX_CLI = 15;
const int LISTENQUEUE = 5;
int threadnum = DEFAULT_THREADNUM;

const char USERNAME[] = "lin";
const char PASSWD[] = "123";


char servpath[MAXLINE] = ".";
vector<string> servfilevec;

int in = 0,out = 0;
vector<int> clientfd(INIT_MAX_CLI,0);
pthread_t* threadptr;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t path_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t filevec_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t client_cond = PTHREAD_COND_INITIALIZER;
	
void* threadFunc (void* arg)									//线程函数	
{
	printf("thread %d %d starting...\n",(int)arg,pthread_self());

	pthread_detach(pthread_self());
	struct sigaction act,oldact;
	act.sa_handler = sigThreadfunc;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGTERM,&act,&oldact);
	while (true)
	{
		pthread_mutex_lock(&client_mutex);
		while (in == out)
			pthread_cond_wait(&client_cond,&client_mutex);
		int connSock = clientfd[out];
		if (++out == INIT_MAX_CLI)
			out = 0;
		pthread_mutex_unlock(&client_mutex);

		communicateWithClient(connSock);
		close(connSock);
	}
	return NULL;
}

void communicateWithClient(int connfd)								//处理客户端命令端口套接字发送的信息
{
	printf("thread %d accept %d...\n",pthread_self(),connfd);
	int dataSock;
	struct sockaddr_in dataCltAddr;
	char line[MAXLINE];
	bool flag = true;
	int passtype = 0;
	int numtodeal = 0;
	string filename;
	while (flag)
	{
    	int nread =	read(connfd,line,MAXLINE);
		if (nread < 0)
		{
			printf("client %d crashed!\n",connfd);
			return;
		}
		else if (nread == 0)
		{
			printf("client %d leave\n",connfd);
			return;
		}

		line[nread] = '\0';
		//printf("%s",line);

		char* spacepos = strchr(line,' ');
		if (spacepos == NULL)
		{
			write(connfd,wrongmsg,strlen(wrongmsg));
			continue;
		}
		int i;
		for (i = 0; i != ServerCMDNUM; ++i)
			if (strncmp(line,ServerCMD[i],spacepos-line) == 0)
				break;
		if (i == ServerCMDNUM)
		{
			write(connfd,wrongmsg,strlen(wrongmsg));
			continue;
		}

		int legaluser,legalpasswd;
		int nwrite = 0;
		char* pos = strstr(spacepos+1,"\r\n");
		switch (i)
		{
			case 0:
				legaluser = strncmp(spacepos+1,USERNAME,pos-spacepos-1);
				break;
			case 1:
				legalpasswd = strncmp(spacepos+1,PASSWD,pos-spacepos-1);
				if (legaluser == 0 && legalpasswd == 0)
				{
					write(connfd,rightmsg,strlen(rightmsg));
					printf("client %d logged in\n",connfd);
				}
				else
					write(connfd,wrongmsg,strlen(wrongmsg));
				break;
			case 2:
				char dataport[10];
				initDataSock(&dataSock,&dataCltAddr);
		//		printf("dataport : %d\n",dataCltAddr.sin_port);
				nwrite = sprintf(dataport,"%d \r\n",dataCltAddr.sin_port);
				dataport[nwrite] = '\0';
				write(connfd,dataport,strlen(dataport));
				
				switch (passtype)
				{
					case 3:
						passFileVec(dataSock);
						break;
					case 4:
						sendFiles(dataSock,filename,numtodeal);
						break;
					case 5:
						recvFiles(dataSock,filename,numtodeal);
						break;
					default:
						printf("cant'be here!\n");
						break;
				}
				close(dataSock);
				break;
			case 3:
				if (pos != spacepos+1)
					write(connfd,wrongmsg,strlen(wrongmsg));
				else
				{
				    write(connfd,rightmsg,strlen(rightmsg));
					passtype = i;
				}
				break;
			case 4:
				filename.clear();
                numtodeal = splitFiles(spacepos+1,filename,1);
				printf("numtodeal %d\n",numtodeal);
				if (numtodeal == 0)
					write(connfd,wrongmsg,strlen(wrongmsg));
				else
				{
					int nwrite = sprintf(line,"%s %d\r\n",rightmsg,numtodeal);
					write(connfd,line,nwrite);
					passtype = i;
				}
				break;
			case 5:
				filename.clear();
                numtodeal = splitFiles(spacepos+1,filename,0);
				printf("numtodeal %d\n",numtodeal);
				if (numtodeal == 0)
					write(connfd,wrongmsg,strlen(wrongmsg));
				else
				{
					write(connfd,rightmsg,strlen(rightmsg));
					passtype = i;
				}
				break;
			case 6:
				close(connfd);
				flag = false;
				printf("client %d logged out\n",connfd);
				break;
			default:
				printf("Can't be here!\n");
				break;
		}
	}
}

void initDataSock(int* datafd,struct sockaddr_in* addr)						//初始化服务器端数据传输套接字
{
	socklen_t addrlen = sizeof(*addr);
	*datafd = socket(AF_INET,SOCK_STREAM,0);
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	bind(*datafd,(struct sockaddr*)addr,addrlen);
	listen(*datafd,1);
	getsockname(*datafd,(struct sockaddr*)addr,&addrlen);
}

void passFileVec(int datafd)									//传输本地文件名列表
{
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);

	char data[MAXSIZE];
	char line[MAXLINE];

	int conndatafd = accept(datafd,(struct sockaddr*)&clientaddr,&len);
	int filenum = servfilevec.size();
	//printf("file num %d\n",filenum);
	
	pthread_mutex_lock(&filevec_mutex);
	strcpy(data,servfilevec[0].c_str());
	int datalen = servfilevec[0].size()+1;
	data[datalen-1]= '\n';
	data[datalen] = '\0';
	for (int i = 1; i != filenum; ++i)
	{
		strcat(data,string(servfilevec[i]+'\n').c_str());
		datalen += servfilevec[i].size()+1;
	}
	pthread_mutex_lock(&filevec_mutex);
	int nwrite = sprintf(line,"%d \r\n",datalen);
	line[nwrite] = '\0';
	
	write(conndatafd,line,nwrite);
	int nread =	read(conndatafd,line,MAXLINE);
	line[nread] = '\0';
	write(conndatafd,data,datalen);
}
	
int splitFiles(char* data,string& filename,int flag)						//将接收到的文件名信息拆分成独立的文件名，保存至filename
{
	char* ptr = data;
	int num = 0;
	while (true)
	{
		while (*ptr == ' ')
			ptr++;
		if (*ptr == '\r')
			break;
		string file;
		char* spaceptr = strchr(ptr,' ');
		if (spaceptr == NULL)
		{
			char* end = strchr(ptr,'\r');
			data[end-data] = '\0';
			file = string(ptr);

			if ((flag && existFile(file)) || (!flag && !existFile(file)))
			{
				filename += file+' '; 
			    ++num;
			}
			break;
		}
		else
			file = string(ptr,spaceptr);   
	
		if ((flag && existFile(file)) || (!flag && !existFile(file)))
		{
			filename += file+' ';
			++num;
		}
		ptr = spaceptr;
	}
	return num;
}


void sendFiles(int datafd,const string& file,int numtodeal)					//向客户端传输文件，numtodeal 传输的文件数
{
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);

	int conndatafd = accept(datafd,(struct sockaddr*)&clientaddr,&len);
	char filemsg[MAXLINE];

	int pos = 0;	
	while (numtodeal--)
	{
		int space = file.find(' ',pos);
		string name = file.substr(pos,space-pos);
		unsigned long sz = getFileSize(name);
		int nwrite = sprintf(filemsg,"%s %ld\r\n",name.c_str(),sz);
		filemsg[nwrite] = '\0';
		write(conndatafd,filemsg,nwrite);
		read(conndatafd,filemsg,MAXLINE);

		doSend(conndatafd,name);
		read(conndatafd,filemsg,MAXLINE);
		pos = space+1;	
	}
	close(conndatafd);
}

void recvFiles(int datafd,const string& file,int numtodeal)					//从客户端接收文件，numtodeal 接收的文件数
{
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);

	int conndatafd = accept(datafd,(struct sockaddr*)&clientaddr,&len);

	char filemsg[MAXLINE];

	int pos = 0;
	while (numtodeal--)
	{
		int space = file.find(' ',pos);
		string name = file.substr(pos,space-pos);
	    int nwrite = sprintf(filemsg,"%s \r\n",name.c_str());
		filemsg[nwrite] = '\0';
		write(conndatafd,filemsg,nwrite);
		
		int nread = read(conndatafd,filemsg,MAXLINE);
		filemsg[nread] = '\0';
		char* ptr = strchr(filemsg,' ');
		filemsg[ptr-filemsg] = '\0';
		unsigned long filesz = atol(filemsg);

		printf("%ld\n",filesz);
		write(conndatafd,rightmsg,strlen(rightmsg));
		doRecv(conndatafd,name,filesz);
		pos = space+1;
	}
	close(conndatafd);
}


void doSend(int datafd,const string& name)							//实际发送文件函数
{
	char data[MAXSIZE];
    FILE* fp = fopen(name.c_str(),"rb");
	long long sum = 0;
	while (true)
	{
		int nread = fread(data,1,MAXSIZE,fp);
		sum += nread;
		write(datafd,data,nread);
		if (nread < MAXSIZE)
			break;
	}

	printf("%ld\n",sum);
	printf("%s send complete!\n",name.c_str());
	fclose(fp);
}
	
void doRecv(int datafd,const string& name,unsigned long filesz)				//实际接收文件函数
{
	char data[MAXSIZE];
	pthread_mutex_lock(&filevec_mutex);
	int fd = open(name.c_str(),O_CREAT | O_WRONLY,S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
	servfilevec.push_back(name);
	pthread_mutex_unlock(&filevec_mutex);

	while (true)
	{
		int nread = read(datafd,data,MAXSIZE);
		write(fd,data,nread);
		if (nread < MAXSIZE)
			break;
	}
	printf("%s receive complete!\n",name.c_str());
	close(fd);
}

bool existFile(const string& file)									//判断当前路径下是否存在某文件
{
	pthread_mutex_lock(&filevec_mutex);
	int len = servfilevec.size();
	for (int i = 0; i != len; ++i)
	{
		if (servfilevec[i] == file)
		{
			pthread_mutex_unlock(&filevec_mutex);
			return true;
		}
	}
	pthread_mutex_unlock(&filevec_mutex);
	return false;
}


void sigFunc(int sig)										//SIGINT 信号的相应函数，向各子线程发送SIGTERM信号	
{
	for (int i = 0; i != threadnum; ++i)
		pthread_kill(threadptr[i],SIGTERM);
	while (threadnum != 0);
	exit(0);
}

void sigThreadfunc(int sig)										//子线程的SIGTERM信号响应函数
{
	printf("thread %d exit~\n",pthread_self());
	pthread_mutex_lock(&num_mutex);
	--threadnum;
	pthread_mutex_unlock(&num_mutex);
	pthread_exit(NULL);
}
