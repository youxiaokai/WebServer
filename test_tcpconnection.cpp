//
// Created by oil_you on 2019/11/12.
//

#include <iostream>
#include <signal.h>
#include "EventLoop.h"
#include "InetAddress.h"
#include <sys/timerfd.h>
#include "Acceptor.h"
#include <stdio.h>
#include <cassert>
#include <arpa/inet.h>
#include "EventLoopThread.h"
#include "TcpConnection.h"
#include "TcpServer.h"

using namespace std;

//class IgnoreSigPipe{
//public:
//    IgnoreSigPipe(){
//        ::signal(SIGPIPE,SIG_IGN);
//    }
//};
//
//IgnoreSigPipe initObj;

void onConnection(const TcpConnection::TcpConnectionPtr& conn){
    if(conn->connected()){
        printf("onConnection():new connection [%s] form %s\n",
               conn->name().c_str(),
               conn->peerAddress().toIpPort().c_str());
        conn->send("Hello world!\n");
        conn->send("I love you!\n");


    }else{
        printf("onConnection()::connection [%s] is down!\n",
               conn->name().c_str());
    }
}

void onMessage(const TcpConnection::TcpConnectionPtr& conn,Buffer* buffer){
    printf("onMessage():received %zd byte from connection [%s]\n",buffer->readableBytes(),
           conn->name().c_str());
}

int main()
{
    printf("main():pid=%d\n",getpid());

    InetAddress listenAddr(12345);
    EventLoop loop;

    TcpServer server(&loop,listenAddr,"hello world!");
    server.setConnectionCallback(&onConnection);
    server.setMessageCallback(&onMessage);
    server.setThreadNum(0);
    server.start();

    loop.loop();

    return 0;
}

