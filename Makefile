ftp: ftpServer.o response.o socketConnection.o
	clang -g ftpServer.o response.o socketConnection.o -o ftp
	
ftpServer.o: ftpServer.c ftpServer.h
	clang -c ftpServer.c ftpServer.h

response.o: response.c response.h
	clang -c response.c response.h

socketConnection.o: socketConnection.c socketConnection.h
	clang -c socketConnection.c socketConnection.h

clean:
	rm *.o ftp