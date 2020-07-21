//
// Created by oil_you on 2019/11/4.
//

#ifndef NETSERVER_ACCEPTOR_H
#define NETSERVER_ACCEPTOR_H


#include <boost/core/noncopyable.hpp>
#include <functional>
#include "InetAddress.h"
#include "EventLoop.h"
#include "Socket.h"

using namespace std;


class Acceptor:boost::noncopyable {
public:
    typedef  function<void(int socket,InetAddress&)> NewConnetionCallback;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnetionCallback& cb){
        newConnetionCallback_=cb;
    };

    bool listenning(){
        return listenning_;
    }

    void listen();

private:
    void handleRead(); //可读回调函数

    EventLoop* loop_;//loop指针
    Socket listenSocket_;//监听套接字
    Channel listenChannel;//和监听套接字绑定的通道
    NewConnetionCallback newConnetionCallback_;//一旦有新连接发生执行的回调函数
    bool listenning_;//acceptChannel所处的EventLoop是否处于监听状态
    int idleFd_;//用来解决文件描述符过多引起电平触发不断触发的问题
};


#endif //NETSERVER_ACCEPTOR_H
