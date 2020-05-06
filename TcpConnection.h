//
// Created by oil_you on 2019/11/7.
//

#ifndef CMAKE_BUILD_DEBUG_NETSERVER_TCPCONNECTION_H
#define CMAKE_BUILD_DEBUG_NETSERVER_TCPCONNECTION_H

#include <string>
#include "EventLoop.h"
#include "Socket.h"
#include "Buffer.h"
#include "Timer.h"

using std::string;

class TcpConnection:boost::noncopyable,
public std::enable_shared_from_this<TcpConnection>{
public:
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;//唯一用shared_ptr管理的class，因为其模糊的生命期
    typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void(const TcpConnectionPtr&, Buffer* buf)> MessageCallback;
    typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
    typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef std::function<void()> Callback;
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

    void setTcpNoDelay(bool on);

    void startRead();
    void stopRead();
    bool isReading() const {return reading_;}

    void setContext(const boost::any& context){context_=context;}
    const boost::any& getContext() const { return context_; }
    boost::any* getMutableContext() { return &context_; }

    void setTimer(Timer* timer){timer_= timer;}
    const Timer* getTimer() const{return timer_;}
    Timer*  getMutableTimer(){return timer_;}

    int sockfd() const {return channel_->fd();}

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

    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE{kConnecting,kConnected,kDisconnecting,kDisconnected};
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleErro();

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

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;

    WriteCompleteCallback writeCompleteCallback_;
//    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

//    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    Timer* timer_;

    boost::any context_;

};

namespace detail{//默认回调函数？？
    void defaultConnectionCallback(const TcpConnection::TcpConnectionPtr& conn);

    void defaultMessageCallback(const TcpConnection::TcpConnectionPtr&,
                                Buffer* buf);
    const void defaultTimerCallback();
}


#endif //CMAKE_BUILD_DEBUG_NETSERVER_TCPCONNECTION_H
