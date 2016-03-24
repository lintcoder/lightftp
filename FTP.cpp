#include "FTP.h"
#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
using std::string;
using std::vector;
const int cmdport = 5600;

const char rightmsg[10] = "OK\r\n";
const char wrongmsg[10] = "WRONG\r\n";
const char* ServerCMD[ServerCMDNUM] = {"USER","PASS","PASV","SHOW","GET","PUT","QUIT"};	//客户端发送给服务器端的命令类型


void setFileVec(vector<string>& filevec,char* path)						//将路径切换到path，同时将新路径下的文件填入filevec
{
	DIR* dir;
	if (strcmp(path,".") != 0)
	{
		if ((dir = opendir(path)) == NULL)
			err_sys("invalid pathname or permission denied!");
		else
			chdir(path);
	}
	else
	{
		char* pwd = getcwd(path,MAXLINE);
		if (pwd == NULL)
			err_sys("fail to get current directory!");
		else
		{
			dir = opendir(path);
			chdir(path);
		}
	}
	struct dirent* item;
	struct stat statbuf;
	while ((item = readdir(dir)) != NULL)
	{
		lstat(item->d_name,&statbuf);
		if (S_ISDIR(statbuf.st_mode))
		{
			if (strcmp(".",item->d_name) == 0 ||
				strcmp("..",item->d_name) == 0)
				continue;
			else
				filevec.push_back(string(item->d_name));
		}
		else
			filevec.push_back(item->d_name);
	}
}

unsigned long getFileSize(const string& file)							//获取某文件的大小（字节数）				
{
	unsigned long filesize;
	struct stat statbuff;
	lstat(file.c_str(),&statbuff);
	filesize = statbuff.st_size;

	return filesize;
}

void printLocalFile(const vector<string>& filevec)											//输出当前路径下的文件名列表
{	
	int len = filevec.size();
	for (int i = 0;i != len; ++i)
		printf("%s\n",filevec[i].c_str());
}

void err_sys(const char* msg)									//打印错误信息并终止程序
{
	printf("%s\n",msg);
	exit(1);
}
