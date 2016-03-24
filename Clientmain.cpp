#include "FTPClient.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using std::cin;
using std::string;

int main(int argc,char* argv[])
{
	if (argc < 2)
		err_sys("usage: ftp <ip> [path]");
	in_addr_t serv_ip;
	if ((argc >= 2 && (serv_ip = inet_addr(argv[1]) == INADDR_NONE)))
		err_sys("invalid ip!");
	
	if (argc >= 3)
		strcpy(clientpath,argv[2]);
													//初始化套接字信息，获得本地路径及文件名列表
	setFileVec(clientfilevec,clientpath);

	int connSock = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(serv_ip);
	servaddr.sin_port = htons(cmdport);

   if(connect(connSock,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0)
	   err_sys("connect error!");

   char line[MAXLINE];
   while (true)											//进行用户名和密码的身份验证
   {
	   char name[20];
	   char passwd[20];
	   printf("username : ");
	   scanf("%s",name);
	   fflush(stdin);
	   int nwrite = sprintf(line,"USER %s\r\n",name);
	   write(connSock,line,nwrite);

	   printf("password : ");
	   scanf("%s",passwd);
	   fflush(stdin);
	   nwrite = sprintf(line,"PASS %s\r\n",passwd);
	   write(connSock,line,strlen(line));

	   int nread = read(connSock,line,MAXLINE);
	   line[nread] = '\0';
	   if (strstr(line,"WRONG") == line)
		   printf("wrong username or password!\n");
	   else
		   break;
   }

   while (cin.get() != '\n');

   bool flag = true;
   while (flag)											//处理相关命令
   {
	   int numtodeal = 0;
	   string filename;
	   fflush(stdin);
	   string str;
	   getline(cin,str);
	   
	   str.erase(str.begin()+str.find_last_not_of(' ')+1,str.end());		//去除str尾部的空格
	   int spacepos = str.find(' ',0);

	   string cmd;										//输入的命令名
	   if (spacepos == string::npos)
		   cmd = str;
	   else
	   	   cmd = str.substr(0,spacepos);

	   int i;
	   for (i = 0; i != ClientCMDNUM; ++i)
			if (cmd == ClientCMD[i])
				break;
	   if (i == ClientCMDNUM)
	   {
		   printf("invalid command!\n");
		   continue;
	   }
	   char* spaceptr;
	   if (i < ClientCMDNUM-2)								//发送给服务器端的命令处理
	    {
		   int nwrite = 0;
		   if (spacepos == string::npos)
			   nwrite = sprintf(line,"%s \r\n",ServerCMD[i]);
		   else
			   nwrite = sprintf(line,"%s %s\r\n",ServerCMD[i],str.substr(spacepos+1).c_str());
		   line[nwrite] = '\0';
		   spaceptr = strchr(line,' ')+1;
		   write(connSock,line,nwrite);
	   }

	   switch(i)
	   {
		   case 3:
			   showFileVec(connSock);
			   break;
		   case 4:
			   downloadFiles(connSock);
			   break;
		   case 5:
			   if (!splitFiles(spaceptr,filename,numtodeal))
				 printf("some files are not found!\n");
			   else
			   	 uploadFiles(connSock,numtodeal);
			   break;
		   case 6:
			   close(connSock);
			   flag = false;
			   break;
		   case 7:
			   printLocalFile(clientfilevec);
			   break;
               case 8:
			   str = str.substr(spacepos+1);
			   if (!changeLocalDir(str))
				printf("invalid directory or permission denied!\n");
			   break;

		   default:
			   printf("can't be here!\n");
			   break;
	   } 
   }

   return 0;
}
