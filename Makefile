all: ftpserv ftpclient clean
ftpserv: Servmain.o FTPServ.o FTP.o
	g++ -o ftpserv Servmain.o FTPServ.o FTP.o -lpthread
Servmain.o: Servmain.cpp FTPServ.h
	g++ -c -D_REENTRANT Servmain.cpp
FTPServ.o: FTPServ.cpp FTPServ.h
	g++ -c -D_REENTRANT FTPServ.cpp
FTP.o: FTP.cpp FTP.h
	g++ -c FTP.cpp

ftpclient: Clientmain.o FTPClient.o FTP.o
	g++ -o ftpclient Clientmain.o FTPClient.o FTP.o
Clientmain.o: Clientmain.cpp FTPClient.h
	g++ -c Clientmain.cpp
FTPClient.o: FTPClient.cpp FTP.h
	g++ -c FTPClient.cpp
clean:
	-rm -f *.o

