//
// Created by oil_you on 2019/11/4.
//

#ifndef NETSERVER_TCPSERVER_H
#define NETSERVER_TCPSERVER_H


#include <boost/core/noncopyable.hpp>
#include <functional>
#include "EventLoop.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include <map>
#include <memory>
#include "TcpConnection.h"


using namespace std;


class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer: boost::noncopyable{
public:
    typedef function<void(EventLoop*)> ThreadInitCallback;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef std::function<void(const TcpConnectionPtr&, Buffer*)> MessageCallback;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg,
            Option option=kNoReusePort);

    ~TcpServer();

    const string& ipPort() const { return ipPort_; }
    const string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }
    shared_ptr<EventLoopThreadPool> threadPool(){return threadPool_;}

 
		//如果不监听，则启动服务器。
	   //
	   //多次调用是无害的。
	   //线程安全。
    void start();

	//设置用于处理输入的线程数。
 //始终在循环线程中接受新连接。
 //必须在@c start之前调用
  // @param numThreads
  //0表示循环线程中的所有I / O，不会创建任何线程。这是默认值。
  //1表示另一个线程中的所有I / O。
  //N表示具有N个线程的线程池，新连接将按循环分配。
    void setThreadNum(int numThreads);//设置server中需要运行多少个Loop线程
    void setThreadInitCallback(const ThreadInitCallback& cb) {threadInitCallback_=cb;}
    void setConnectionCallback(const ConnectionCallback& cb) {connectionCallback_=cb;}//设置连接回调
    void setMessageCallback(const MessageCallback& cb){messageCallback_=cb;}//设置消息回调
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {writeCompleteCallback_=cb;}//设置写完成回调

private:
	/// Not thread safe, but in loop
  //被设置为Acceptor::newConnectionCallback()回调函数
  //创建TcpConnection对象conn，把它加入connectionmap，设置好callback
    void newConnection(int sockfd,const InetAddress& peerAddr);

    void removeConnection(const TcpConnectionPtr& conn);//将conn从ConnectionMap中移除

    void removeConnectionInLoop(const TcpConnectionPtr& conn);//使用map关联容器维护一个连接列表，每个连接有一个唯一的名字，在创建时生成

    typedef map<string,TcpConnectionPtr> ConnectionMap;
    ConnectionMap connections_;//连接列表

    EventLoop* loop_;
    const string ipPort_;//服务器地址+端口号
    const string name_;//服务器名字

    unique_ptr<Acceptor> acceptor_;// avoid revealing Acceptor //用于接受连接的Acceptor
    shared_ptr<EventLoopThreadPool> threadPool_;//线程池对象

    ConnectionCallback connectionCallback_; //新连接回调
    MessageCallback messageCallback_; //消息回调
    WriteCompleteCallback writeCompleteCallback_;  //写完成回调
    ThreadInitCallback threadInitCallback_; //线程初始化回调？？
    bool started_;  //启动标记

    int nextConnId_;//下一个连接ID


};


#endif //NETSERVER_TCPSERVER_H
