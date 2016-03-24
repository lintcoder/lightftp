#include "FTPClient.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <dirent.h>
#include <fcntl.h>

using std::string;
using std::vector;

const char* ClientCMD[ClientCMDNUM] = {"USER","PASS","PASV","ls","get","put","bye","lcd","cd"};
char clientpath[MAXLINE] = ".";
vector<string> clientfilevec;

void showFileVec(int connfd)										//显示服务器端当前路径下的文件
{
	char line[MAXLINE];
	int nread = read(connfd,line,MAXLINE);
	line[nread] = '\0';

	if (strstr(line,"WRONG") == line)
	{
		printf("invalid command! (usage:ls)\n");
		return;
	}

	int dataport = passPasvCmd(connfd,line);
	
	//printf("%d\n",dataport);
	int dataSock = connectDatasock(connfd,dataport);
	nread = read(dataSock,line,MAXLINE);
	line[nread] = '\0';
	char* space = strchr(line,' ');
	line[space-line] = '\0';
	int datalen = atoi(line);

	write(dataSock,rightmsg,strlen(rightmsg));
	char data[MAXSIZE];
	string file;
	while (true)
	{
		nread = read(dataSock,data,MAXSIZE);
		data[nread] = '\0';
		file += string(data);
		if (nread < MAXSIZE-1)
			break;
	}

	int start = 0;
	while (start < datalen)
	{
		int pos = file.find('\n',start);
		//cout<<file.substr(start,pos-start)<<endl;
		printf("%s\n",file.substr(start,pos-start).c_str());
		start = pos+1;
	}
	close(dataSock);
}

int passPasvCmd(int connfd,char* line)									//通知服务器以被动方式进行数据传输，并获得数据传输的端口号
{
	int i;
	for (i = 0; i != ClientCMDNUM; ++i)
		if (strcmp(ClientCMD[i],"PASV") == 0)
			break;

	int nwrite = sprintf(line,"%s \r\n",ClientCMD[i]);
	line[nwrite] = '\0';
	write(connfd,line,nwrite);
	int nread = read(connfd,line,MAXLINE);
	line[nread] = '\0';
	char* space = strchr(line,' ');
	line[space-line] = '\0';
	
	int dataport = atoi(line);

	return dataport;
}

void downloadFiles(int connfd)										//客户端下载文件
{
    char line[MAXSIZE];
	int nread = read(connfd,line,MAXLINE);
	line[nread] = '\0';

	if (strstr(line,"WRONG") == line)
	{
		printf("invalid command! (usage:get filename...)\n");
		return;
	}

	char* space = strchr(line,' ')+1;
	char* end = strchr(line,'\r');
	line[end-line] = '\0';
	int numtodeal = atoi(space);

	int dataport = passPasvCmd(connfd,line);

	int dataSock = connectDatasock(connfd,dataport);
	while (numtodeal--)
	{
		int nread = read(dataSock,line,MAXLINE);
		line[nread] = '\0';
		space = strchr(line,' ');
		line[space-line] = '\0';
		//string name = string(line);
		end = strchr(space+1,'\r');
		line[end-line] = '\0';
		unsigned long filesz = atol(space+1);

		write(dataSock,rightmsg,strlen(rightmsg));
		
		doDownload(dataSock,line,filesz);
		write(dataSock,rightmsg,strlen(rightmsg));
	}
	close(dataSock);
}

void doDownload(int datafd,const char* path,unsigned long filesz)					//实际的下载函数
{
	int filefd = open(path,O_CREAT | O_WRONLY,S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH |S_IWOTH);
	unsigned long sz = 0;
	char data[MAXSIZE];
	while (true)
	{
		int nread = read(datafd,data,MAXSIZE);
//		printf("%d\n",nread);
		write(filefd,data,nread);
		sz += nread;
		if (nread < MAXSIZE)
			break;
	}
	printf("%s %ld received\n",path,sz);
	close(filefd);
}

void uploadFiles(int connfd,int numtodeal)								//客户端上传文件
{
	char line[MAXLINE];
	int nread = read(connfd,line,MAXLINE);
	if (strstr(line,"WRONG") == line)
	{
		printf("files already on the server!\n");
		return;
	}
	int dataport = passPasvCmd(connfd,line);

	int dataSock = connectDatasock(connfd,dataport);

	while (numtodeal--)
	{
		int nread = read(dataSock,line,MAXLINE);
		line[nread] = '\0';
		//printf("%s %d\n",line,nread);
		char* space = strchr(line,' ');
		line[space-line] = '\0';
	    string name = string(line);	
		printf("%s\n",name.c_str());
		unsigned long filesz = getFileSize(line);
		printf("%ld\n",filesz);
		int nwrite = sprintf(line,"%ld \r\n",filesz);
		line[nwrite] = '\0';
		write(dataSock,line,nwrite);
		read(dataSock,line,MAXLINE);

		doUpload(dataSock,name);
	}
}

void doUpload(int datafd,const string& name)								//实际的上传函数
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
	fclose(fp);
}

int connectDatasock(int connfd,int dataport)								//客户端连接服务器端的数据传输套接字
{
	struct sockaddr_in connAddr;
	memset(&connAddr,0,sizeof(connAddr));
	socklen_t addrlen = sizeof(connAddr);

	getpeername(connfd,(struct sockaddr*)&connAddr,&addrlen);
	connAddr.sin_port = dataport;
	int dataSock = socket(AF_INET,SOCK_STREAM,0); 
	int res = connect(dataSock,(struct sockaddr*)&connAddr,sizeof(connAddr));

	return dataSock;
}

bool splitFiles(char* data,string& filename,int& numtodeal)				//将命令中的文件名串拆分成独立的文件名填入filename，numtodeal 实际处理的文件数
{																 
	//printf("%s",data);
	bool flag = true;
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

			if (existFile(file))
			{
				filename += file+' '; 
			    ++num;
			}
			else
			    flag = false;
			break;
		}
		else
			file = string(ptr,spaceptr);   
	
		if (existFile(file))
		{
			filename += file+' ';
			++num;
		}
    	else
			flag = false;
		ptr = spaceptr;
	}
	numtodeal = num;
	return flag;
}

bool existFile(const string& file)										//判断当前路径下某文件是否存在
{
	return access(file.c_str(),4) != -1;
}


bool changeLocalDir(string& dir)										//更改客户端的当前路径
{
	char line[MAXLINE];
	strncpy(line,dir.c_str(),MAXLINE-1);

	if (strcmp(line,".") != 0)
	{
		DIR* dp;
		if ((dp = opendir(line)) == NULL)
			return false;
		else
		{
			chdir(line);
			vector<string> file;
			setFileVec(file,line);
			swap(file,clientfilevec);
			
			char* pwd = getcwd(line,MAXLINE);
			strcpy(clientpath,pwd);
			return true;
		}
	}
	return true;
}	
