//
// Created by oil_you on 2019/11/2.
//

#ifndef NETSERVER_SOCKET_H
#define NETSERVER_SOCKET_H

#include <netinet/in.h>
#include "InetAddress.h"
#include <sys/socket.h>

namespace socketOps{
    int getSocketError(int sockfd);
    void toIp(char* buf,size_t size,
            const struct sockaddr* addr);
    void toIpPort(char* buf, size_t size,
                  const struct sockaddr* addr);

    inline uint16_t networkToHost16(uint16_t net16)
    {
        return be16toh(net16);//16字节网络转主机
    }

    struct sockaddr_in6 getLocalAddr(int sockfd);
    struct sockaddr_in6 getPeerAddr(int sockfd);
}

class InetAddress;

class Socket {
public:
    explicit Socket(int socket)
    :socketfd_(socket)
    {};

    ~Socket();

    int fd() const{
        return socketfd_;
    }


    int creatNonBlockingSocket(sa_family_t family);
    void setNonBlockAndCloseOnExec(int sockfd);

    void bindAddress(const InetAddress& addr);
    int connect(int socket,const struct sockadrr* addr);
    void listen();
    int accept(InetAddress* peeraddr);
    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddress(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);


private:
    int socketfd_;

};


#endif //NETSERVER_SOCKET_H
