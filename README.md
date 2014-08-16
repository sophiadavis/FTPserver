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

Unfortunately, different threads cannot have different working directories. I worked around this by assigning a 'virtual' working directory to each thread/client, in the form of a string representing the path to where the client on this thread "thinks" it is. This makes changing directory a bit tricky. Instead of re-implementing path-parsing, when a CD command is issued, I use a mutex to lock access to the process working directory. While the resource is locked, I change the process directory from the initial root directory to where the thread 'thinks' it is, and from there to the requested directory. The absolute path of the requested directory is saved as the client's current working directory. Finally, the process directory is changed back to the initial root directory, and the lock is released. All other NLST and RETR commands are issued relative to the virtual path. It's definitely a hack, but it seems to work.

To run:  
`make`  
`mv server /var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server` (create pretend 'remote' file system)  
`./ftp`  

And connect with your FTP client (it's `ftp localhost 5000` on my machine.)   

Login with the username `anonymous`.