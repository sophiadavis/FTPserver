### FTP Server

----------

Sophia Davis  
Summer 2014

----------

This simple FTP server handles the following commands:  

* USER   
* QUIT  
* PWD
* CWD
* PASV
* NLST
* RETR
* TYPE
* SYST
* FEAT

It succesfully sends files to the built-in FTP client on my laptop, using passive mode.    
	
The server socket is set to listen on localhost, port 5000, and only handles IPv4 connections. It assumes that the directory '/var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server' exists, and the process is chroot jailed to that directory (I've commented these lines out to avoid having to execute my program with sudo).

A new thread is opened for each client. One socket is used for exchange of FTP codes and messages, and another set of sockets is opened on a separate port for each data transaction. All data is transfered in binary mode. The server assumes all commands consist of one or two words. When the client issues an unrecognized command, the server responds with '502 Command not implemented.'

Unfortunately, threads do not support different working directories. I started creating 'virtual' working directories by relating each thread with a string representing the path to the thread's wd, and then issuing all other NLST and RETR commands relative to this path. My (messy) progress with basic concatenation of paths is in thread_wd_initial.c
I might return to this later, but for now I think I've had enough with string manipulation in C, and I won't be able to parse paths as elegantly as I would like.

To run:  
```make```  
```mv server /var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server``` (create pretend 'remote' file system)  
```./ftp```

And connect with your FTP client (```ftp localhost 5000``` on my machine.) Login with USER anonymous.