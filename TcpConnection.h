//
// Created by oil_you on 2019/11/7.
//

#ifndef CMAKE_BUILD_DEBUG_NETSERVER_TCPCONNECTION_H
#define CMAKE_BUILD_DEBUG_NETSERVER_TCPCONNECTION_H

#include <string>
#include "EventLoop.h"
#include "Socket.h"
#include "Buffer.h"

using std::string;

class TcpConnection:boost::noncopyable,
public std::enable_shared_from_this<TcpConnection>{
public:
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;//唯一用shared_ptr管理的class，因为其模糊的生命期
    typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void(const TcpConnectionPtr&, Buffer* buf)> MessageCallback;
    typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
    typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
//    typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

    TcpConnection(EventLoop* loop,
            const string& name,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr);

    ~TcpConnection();

    EventLoop* getLoop() const {return loop_;}
    const string& name(){ return name_;}
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const {return state_==kConnected;}
    bool disconnected() const { return state_==kDisconnected;}

    void send(const string& message);
    void send(const void* message, int len);
    void send(Buffer* buf);

    void shutdown();
    void forceClose();
    //void forceCloseWithDelay(double seconds);

    void setTcpNoDelay(bool on);//禁用Nagle算法，避免连续发包出现延迟，这对编写低延迟网络服务很重要

    void startRead();
    void stopRead();
    bool isReading() const {return reading_;}

    void setContext(const boost::any& context){context_=context;}
    const boost::any& getContext() const { return context_; }
    boost::any* getMutableContext() { return &context_; }

    int sockfd() const {return channel_->fd();}

	//以下接口为设置连接对应的各种回调:
    void setConnectionCallback(const ConnectionCallback& cb){ connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){ writeCompleteCallback_ = cb; }
//    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,size_t highWaterMark){
//        highWaterMarkCallback_=cb;
//        highWaterMark_=highWaterMark;
//    }
    void setCloseCallback(const CloseCallback& cb){closeCallback_=cb;}

    Buffer* inputBuffer(){return &inputBuffer_;}
    Buffer* outputBuffer(){return &outputBuffer_;}

	// called when TcpServer accepts a new connection
    void connectEstablished();// should be called only once
	// called when TcpServer has removed me from its map
    void connectDestroyed();// should be called only once

private:
    enum StateE{kConnecting,kConnected,kDisconnecting,kDisconnected};//对于一个连接的从生到死进行了状态的定义，类似一个状态机 //分别代表：已经断开、初始状态、已连接、正在断开
    void handleRead();//处理读事件
    void handleWrite();//处理写事件
    void handleClose(); //处理关闭事件
    void handleErro();  //处理错误事件

    void sendInLoop(const string& message);
    void sendInLoop(const void* message, size_t len);

    void shutdownInLoop();
    void forceCloseInLoop();
    void setState(StateE s){state_=s;}
    const char* stateToString() const;

    void startReadInLoop();
    void stopReadInLoop();

    EventLoop* loop_;
    const string name_;
    StateE state_;
    bool reading_;

    unique_ptr<Socket> socket_; //连接对应的套接字
    unique_ptr<Channel> channel_;  //对应的事件分发器channel
    const InetAddress localAddr_;  //本地地址
    const InetAddress peerAddr_;  //外接地址

	 //关注三个半事件  (这几个回调函数通过handle**那四个事件处理函数调用)
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;

    WriteCompleteCallback writeCompleteCallback_;
//    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

//    size_t highWaterMark_;

    Buffer inputBuffer_;//输入缓冲区
    Buffer outputBuffer_;//输出缓冲区

    boost::any context_;

};

namespace detail{//默认回调函数？？
    void defaultConnectionCallback(const TcpConnection::TcpConnectionPtr& conn);

    void defaultMessageCallback(const TcpConnection::TcpConnectionPtr&,
                                Buffer* buf);
}


#endif //CMAKE_BUILD_DEBUG_NETSERVER_TCPCONNECTION_H
