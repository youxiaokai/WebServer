//
// Created by oil_you on 2019/11/4.
//

#include <fcntl.h>
#include "Acceptor.h"
#include "Socket.h"
#include "assert.h"

class Socket;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
:loop_(loop),
listenSocket_(listenSocket_.creatNonBlockingSocket(listenAddr.family())),//创建监听套接字
listenChannel(loop_,listenSocket_.fd()), //绑定Channel和socketfd
listenning_(false),
idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))//预先准备一个空闲文件描述符

{
    assert(idleFd_>=0);
    listenSocket_.setReusePort(reuseport);
    listenSocket_.setReuseAddress(true);
    listenSocket_.bindAddress(listenAddr);
    listenChannel.setReadCallback(
            bind(&Acceptor::handleRead,this)
            );//设置Channel的fd的读回调函数
}

Acceptor::~Acceptor() {
    listenChannel.disableAll();
    listenChannel.remove();
    close(idleFd_);
}

void Acceptor::listen() {
    listenning_=true;
    listenSocket_.listen();//开启监听
    listenChannel.enableReading(); //关注可读事件
}

void Acceptor::handleRead() {
    InetAddress peerAddr;
    //cout<<"Acceptor::handleRead() "<<listenChannel.fd()<<endl;
    int connfd=listenSocket_.accept(&peerAddr);//获得已连接套接字
    if(connfd>=0){
        if(newConnetionCallback_){//如果设置了新连接回调函数
            newConnetionCallback_(connfd,peerAddr); //那么就执行它
        }
        else{
            close(connfd);//否则就关闭，sockets是全局函数
        }
    }
    else{ //如果<0失败了?
        perror("in Acceptor::handleRead ");
        if (errno == EMFILE) //太多的文件描述符
        {
            ::close(idleFd_); //先关闭空闲文件描述符，让它能够接收。否则由于采用电平触发，不接收会一直触发。
            idleFd_ = ::accept(listenSocket_.fd(), NULL, NULL);//那就腾出一个文件描述符，用来accept
            ::close(idleFd_);//accept之后再关闭
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC); //然后再打开成默认方式
        }
    }
}
