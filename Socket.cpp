//
// Created by oil_you on 2019/11/2.
//

#include "Socket.h"
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <assert.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

int socketOps::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

void socketOps::toIp(char *buf, size_t size, const struct sockaddr *addr) {
    if (addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);//将32位IPv4的地址， 使用10进制+句点表示时，所占用的char * 数组的长度
        const struct sockaddr_in* addr4 =  static_cast<const struct sockaddr_in*>((const void*)addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));//将地址从数值格式转换到表达式格式
    }
    else if (addr->sa_family == AF_INET6)
    {
        assert(size >= INET6_ADDRSTRLEN);//用于IPv6十六进制字符串
        const struct sockaddr_in6* addr6 = static_cast<const struct sockaddr_in6*>((const void*)addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));//将地址从数值格式转换到表达式格式
    }
}

void socketOps::toIpPort(char *buf, size_t size, const struct sockaddr *addr) {
    toIp(buf,size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in* addr4 = static_cast<const struct sockaddr_in*>((const void*)addr);
    uint16_t port = socketOps::networkToHost16(addr4->sin_port);//16字节网络转主机
    assert(size > end);
    snprintf(buf+end, size-end, ":%u", port);
}

//获得本地地址,getsockname
struct sockaddr_in6 socketOps::getLocalAddr(int sockfd) {
    struct sockaddr_in6 localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, (struct sockaddr*)(&localaddr), &addrlen) < 0)
    {
        perror("socketOps::getLocalAddr");
    }
    return localaddr;
}

//获取对等方地址getpeername
struct sockaddr_in6 socketOps::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, (struct sockaddr*)(&peeraddr), &addrlen) < 0)
    {
        perror("socketOps::getPeerAddr");
    }
    return peeraddr;
}




Socket::~Socket() {
    close(socketfd_);
//    cout<<"Socket::~Socket()：socket "<< socketfd_<<" closed"<<endl;
}

void Socket::setNonBlockAndCloseOnExec(int sockfd){
    int flags=::fcntl(sockfd,F_GETFL,0);
    flags|=O_NONBLOCK;
    int ret=::fcntl(sockfd,F_SETFL,flags);

    flags=::fcntl(sockfd,F_GETFD,0);
    flags|=FD_CLOEXEC;
    ret=::fcntl(sockfd,F_SETFD,flags);

    (void) ret;
};

int Socket::creatNonBlockingSocket(sa_family_t family){
    int socketfd=socket(family,SOCK_STREAM,IPPROTO_TCP);
    if(socketfd<0){
        perror("Socket::creatNonBlockingSocket:socket creat fail!");
        exit(-1);
    }
//    std::cout << "Socket::creatNonBlockingSocket：" << socketfd<< std::endl;

    setNonBlockAndCloseOnExec(socketfd);

    return socketfd;
}

void Socket::bindAddress(const InetAddress& addr) {
    int ret=::bind(socketfd_,addr.getSockAddr(),static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
//    cout<<"Socket::bindAddress"<<endl;
    if(ret<0){
        perror("Socket::bindAddress fail!");
    }
}

int Socket::connect(int socket, const struct sockadrr *addr) {
    return ::connect(socket, reinterpret_cast<const sockaddr *>(addr), static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}//成功返回0

void Socket::listen() {
    int ret=::listen(socketfd_,SOMAXCONN);//SOMAXCONN定义了系统中每一个端口最大的监听队列的长度,这是个全局的参数,默认值为128
//    std::cout << "Socket::listen()" << std::endl;
    if(ret<0){
        perror("Socket::listen fail!");
        exit(-1);
    }
}

int Socket::accept(InetAddress* peeraddr) {
    const sockaddr* addr=peeraddr->getSockAddr();
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    int connfd=::accept(socketfd_, const_cast<sockaddr *>(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);

    if (connfd < 0)
    {
        //由于下面某些错误可能会改变errno number，所以先把它保存起来
        int savedErrno = errno;
        perror("Socket::accept");
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;//上述错误不致命，只保存起来就可以了
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors//致命错误直接FATAL
                perror("unexpected error of ::accept");
                exit(-1);
            default:
                perror("unknown error of ::accept");
                break;
        }
    }
    return connfd;
}

void Socket::shutdownWrite() {
    if(::shutdown(socketfd_,SHUT_WR)<0){
        perror("Socket::shutdownWrite fail!");
        exit(-1);
    }
}

void Socket::setTcpNoDelay(bool on) {
#ifdef TCP_NODELAY//默认没有定义，所以下面的语句无效
    int optval=on?1:0;
    ::setsockopt(socketfd_,IPPROTO_TCP,TCP_NODELAY,
            &optval, static_cast<socklen_t>(sizeof optval));
#else
    if(on){
        perror("TCP_NODELAY is not supported");
    }
#endif
}

void Socket::setReuseAddress(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(socketfd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        perror("SO_REUSEPORT failed.");
    }
#else
    if (on)
  {
    perro("SO_REUSEPORT is not supported.");
  }
#endif
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(socketfd_, SOL_SOCKET, SO_KEEPALIVE,
                 &optval, static_cast<socklen_t>(sizeof optval));
}





