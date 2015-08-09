

Improve-UDP

Improve the quality of P2P transmission, UDP protocol transmission speed and reliability of TCP protocol, that is, in the UDP protocol to add to receive data validation, receiving data inspection and sorting of receiving data, the flow of control and other essential elements, in the case of UDP protocol should have speed, greatly improve the transmission reliability of UDP protocol.

API Function

    int sock_init(char *ip, int port, struct sockaddr_in *addr) 
        Function: sock_init Description: create socket and initialization sockaddr , bind ip with port. 
        Input : 
                   ip : specifies a dest-ip , if NULL means listen anyone's connect. 
                   port : must sepcifies port. 
                   addr : address sepcified by addr to the socket referred to bythe file descriptor sockfd. 
        Return : 
                   OK(1) means init sucess or NG (-1) is fail, the error message of reason is print /mnt/syslog

    int start_udp_recv(int fd, struct sockaddr_in *addr ,int *id) 
        Function: start_udp_recv Description: submit a request to the udp_server, if OK, Server will dispatch inline-thread to work. 
        Input : 
                   fd   : socket referred to bythe file descriptor sockfd. 
                   addr : address sepcified by addr. 
                   id   : process ID of the calling process. 
        Return : 
                   OK(1) means request sucess or NG (-1) is fail, the error message of reason is print /mnt/syslog

    int udp_recv(int msqid, void *msgpi, struct sockaddr_in *sock_addr, int sta) 
        Function: udp_recv Description: the function has the same effect as recvmsg.
        Input : 
                   msqid     : system V message queue with identifier msqid. 
                   sock_addr : scokaddr of sender. 
                   sta       : 0 or IPC_NOWAIT, If IPC_NOWAIT is specified in msgflg, then the call instead fails with the error EAGAIN. 
        Return : 
                   if sucess the number of bytes actually copied into the buf or NG (-1) is fail, the error message of reason is print /mnt/syslog

    ssize_t udp_send(int fd, const void *outbuf, size_t outsize, struct sockaddr_in *dest, socklen_t addrlen) 
        Function: udp_send Description: he function has the same effect as sendto. 
        Input : 
                   fd      : socket referred to bythe file descriptor sockfd. 
                   outbuf  : carried the data. dest : address sepcified by dest-addr. 
                   addrlen : the size of struct sockaddr_in. 
        Return : 
                   if sucess the number of bytes actually send into the receiver or NG (-1) is fail, the error message.

Version 

         1.0.0 2015-07-04 12：21：20     First upload.
         1.0.1 2015-08-01 08：12：23     Add daemon attr.

