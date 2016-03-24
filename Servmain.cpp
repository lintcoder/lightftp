#include "FTPServ.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

int main(int argc,char* argv[])
{
	if (argc >= 4)
		err_sys("usage: ftpserv [path] [threads]!");

	if (argc > 1)
		strcpy(servpath,argv[1]);
	if (argc > 2)
		threadnum = atoi(argv[2]);
	setFileVec(servfilevec,servpath);										//初始化套接字信息，获取本地路径和当前路径下的文件名列表

	printf("serv directory is %s\n",servpath);
	int servSock = -1;												
	if((servSock = socket(AF_INET,SOCK_STREAM,0)) < 0)
		err_sys("socket error!");

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(cmdport);
	if (bind(servSock,(struct sockaddr*) &servaddr,sizeof(servaddr)) < 0)
		err_sys("bind error!");

	listen(servSock,LISTENQUEUE);
	threadptr = (pthread_t*)malloc(threadnum*sizeof(pthread_t));
	if (threadptr == NULL)
		err_sys("alloc threads failed!");

	for (int i = 0; i != threadnum; ++i)									//预先启动所有线程
		pthread_create(&threadptr[i],NULL,threadFunc,(void*)i);

	struct sockaddr_in clitaddr;
	socklen_t addrlen = sizeof(clitaddr);

	struct sigaction act,oldact;											//设置信号响应函数
	act.sa_handler = sigFunc;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT,&act,&oldact);

	int connSock;
	int nfds = servSock+1;
	fd_set inputs,testfds;
	FD_ZERO(&inputs);
	FD_SET(0,&inputs);
	FD_SET(servSock,&inputs);
	char line[MAXLINE];

	while (true)
	{
		testfds = inputs;
		int nread = 0;
		int result = select(nfds,&testfds,NULL,NULL,NULL);
		if (FD_ISSET(0,&testfds))
		{
			--result;
			ioctl(0,FIONREAD,&nread);
			read(0,line,nread);
			if (line[nread-1] != '\n')
				continue;
			line[nread-1] = 0;
			if (strcmp(line,"lcd") == 0)
				printLocalFile(servfilevec);
			else if (strcmp(line,"exit") != 0)
			{
				printf("invalid command!\n");
				continue;
			}
			else
				kill(getpid(),SIGINT);
		}
		if (result > 0)
		{
			socklen_t clitlen = addrlen;
			connSock = accept(servSock,(struct sockaddr*)&clitaddr,&clitlen);
			pthread_mutex_lock(&client_mutex);

			clientfd[in] = connSock;
			if (++in == INIT_MAX_CLI)
				in = 0;
			if (in == out)
			{
				printf("no available thread!\n");	
				close(connSock);
			}
			pthread_cond_signal(&client_cond);
			pthread_mutex_unlock(&client_mutex);
		}
	}

	return 0;
	
}
