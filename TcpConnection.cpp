//
// Created by oil_you on 2019/11/7.
//

#include "TcpConnection.h"
#include <assert.h>
#include <iostream>
#include <error.h>

void detail::defaultConnectionCallback(const TcpConnection::TcpConnectionPtr& conn)
{
    cout<< conn->localAddress().toIpPort() << " -> "
        << conn->peerAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}
//默认回调函数？？
void detail::defaultMessageCallback(const TcpConnection::TcpConnectionPtr&,
                            Buffer* buf)
{
    buf->retrieveAll();
}

const void detail::defaultTimerCallback() {
    cout<<"connection time out!"<<endl;
}

TcpConnection::TcpConnection(EventLoop *loop,
        const string& nameArg,
        int sockfd,
        const InetAddress &localAddr,
        const InetAddress &peerAddr)
:loop_(loop),
name_(nameArg),
state_(kConnecting),
reading_(true),
socket_(new Socket(sockfd)),
channel_(new Channel(loop,sockfd)),
localAddr_(localAddr),
peerAddr_(peerAddr),
timer_(new Timer(0, Timer::TimerType::TIMER_ONCE, std::bind(&detail::defaultTimerCallback)))
// highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead,this)
            );
    channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite,this)
            );
    channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose,this)
            );
    channel_->setErroCallback(
            std::bind(&TcpConnection::handleErro,this)
            );
//    cout<< "TcpConnection::constructor[" <<  name_ << "] at " << this
//              << " fd=" << sockfd<<endl;//this显示为内存
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
//    cout << "TcpConnection::destructor[" <<  name_ << "] at " << this
//              << " fd=" << channel_->fd()
//              << " state=" << stateToString();
    delete(timer_);
    timer_=nullptr;
    assert(state_==kDisconnected);
}

void TcpConnection::send(const string &message) {
    if(state_==kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(message);
        }
        else{
            void (TcpConnection::*fp)(const string& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                    std::bind(fp,
                              this,     // FIXME
                              message));//??什么用法
        }
    }

}

void TcpConnection::send(Buffer *buf) {
    if(state_==kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(buf->peek(),buf->readableBytes());
            buf->retrieveAll();
        }
        else{
            void (TcpConnection::*fp)(const string& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                    std::bind(fp,
                              this,     // FIXME
                              buf->retrieveAllAsString()));
            //std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const string &message) {
    sendInLoop(message.c_str(),message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        cout << "disconnected, give up writing";
        return;
    }
    // if no thing in output queue, try writing directly
    //如果输出队列中没有任何内容，请直接尝试写入
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)//如果一次发送完毕就不用启用writeCallback
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                perror("TcpConnection::sendInLoop") ;
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }
//如果只发送了部分数据，把剩余的数据放入outputBuffer_,并开始关注writable事件，以后在handlerWrite()中发送剩余的数据
    assert(remaining <= len);
    if (!faultError && remaining > 0)
    {
//        size_t oldLen = outputBuffer_.readableBytes();
//        //如果输出缓冲的长度超过用户指定的大小，就会触发回调
//        if (oldLen + remaining >= highWaterMark_
//            && oldLen < highWaterMark_
//            && highWaterMarkCallback_)
//        {
//            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
//        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {//关闭该链接写端
    if(state_==kConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if(!channel_->isWriting()){
        socket_->shutdownWrite();//最终会调用系统API shutdown，关闭连接的写这一半
    }
}

void TcpConnection::forceClose() {
    if(state_==kConnected||state_==kDisconnecting){
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop,this));
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if(state_==kConnected||state_==kDisconnecting){
        handleClose();
    }
}

const char* TcpConnection::stateToString() const {
    switch (state_)
    {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop,this));
}

void TcpConnection::startReadInLoop() {
    if(!reading_||!channel_->isReading()){
        channel_->enableReading();
        reading_=true;
    }
}

void TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop,this));
}

void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading())
    {
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();//断言处于loop线程
    assert(state_==kConnecting);//断言处于未连接状态
    setState(kConnected);//将状态设置为已连接

    channel_->enableReading();//一旦连接成功就关注它的可读事件，加入到Poller中关注

    connectionCallback_(shared_from_this());//用户提供的回调函数

}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if(state_==kConnected){
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());//回调用户函数
    }
    channel_->remove();//从通道和Poller中移除
}

void TcpConnection::handleRead() {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n>0){
        messageCallback_(shared_from_this(),&inputBuffer_);
    }
    else if(n==0){
        handleClose();
    }
    else{
        errno = savedErrno;
        perror("TcpConnection::handleRead");
        handleErro();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if(channel_->isWriting()){
        ssize_t n = write(channel_->fd(),
                                   outputBuffer_.peek(),
                                   outputBuffer_.readableBytes());
        if(n>0){
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else{
            perror("TcpConnection::handleWrite");
        }
    }
    else{//??为什么不handleclose
        std::cout<<"Connection fd = "<<channel_->fd()
        <<" is down, no more writing"<<endl;
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
//    cout<<"TcpConnection::handleClose():fd= "<<channel_->fd()<<" state="<<stateToString()<<endl;
    assert(state_==kConnected||state_==kDisconnecting);

    setState(kDisconnected);//设置状态为kDisconnected,表示已断开
    channel_->disableAll();//移除注册的事件,使用epoll时是EPOLL_CTL_DEL

    TcpConnectionPtr guardThis(shared_from_this());//延长本对象的生命周期,引用技术为2

    //用户设置的回调函数，
    connectionCallback_(guardThis);//参数为shared_ptr,保证了 connectionCallback_能安全的使用本对象

    closeCallback_(guardThis);// 调用TcpServer::removeConnection，在TCPServer中设定

}

void TcpConnection::handleErro() {
    int err=socketOps::getSocketError(channel_->fd());
    std::cout<< "TcpConnection::handleError [" << name_
                                      << "] - SO_ERROR = " << err << " " << strerror(err);
}
