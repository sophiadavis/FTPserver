# Echo client program
import socket
import sys

HOST = 'localhost'    # The remote host
PORT = 65500              # The same port as used by the server

def open_socket(port):
    for res in socket.getaddrinfo(HOST, port, socket.AF_UNSPEC, socket.SOCK_STREAM):
        af, socktype, proto, canonname, sa = res
        try:
            s = socket.socket(af, socktype, proto)
        except socket.error as msg:
            s = None
            continue
        try:
            s.connect(sa)
        except socket.error as msg:
            s.close()
            s = None
            continue
        break
    if s is None:
        print 'could not open socket'
        sys.exit(1)
    return s
    
    
    
def main():
    s = open_socket(PORT)
    data = s.recv(1024)
    #     print 'Received', repr(data)

    s.sendall('USER anonymous')
    data = s.recv(1024)
    #     print 'Received', repr(data)

    for i in xrange(70000):
        print i
        s.sendall('PASV')
        data = s.recv(1024)
        print 'Received', repr(data)
        
        port = PORT + i + 1
        print port
        newsock = open_socket(port)
        s.sendall("NLST")
        while 1:
            data = newsock.recv(1024)
            print 'Received', repr(data)
            if not data: break
            
        newsock.close()
    
    #     print 'Received', repr(data)

    s.sendall('QUIT')
    s.close()

if __name__ == "__main__":
    main()



