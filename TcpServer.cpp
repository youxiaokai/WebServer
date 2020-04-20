//
// Created by oil_you on 2019/11/4.
//

#include "TcpServer.h"
#include <assert.h>

using namespace std;


TcpServer::TcpServer(EventLoop *loop,
        const InetAddress &listenAddr,
        const string &nameArg,
        TcpServer::Option option)
        :loop_(loop),//不能为空，否则触发FATAL
        ipPort_(listenAddr.toIpPort()), //端口号
        name_(nameArg), //名称
        acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)),//创建Acceptor，使用scoped_ptr管理
        threadPool_(new EventLoopThreadPool(loop,name_)),//I/O线程池
        connectionCallback_(detail::defaultConnectionCallback),
         messageCallback_(detail::defaultMessageCallback),
         started_(false),
         nextConnId_(1)//下一个已连接编号id
         {
    acceptor_->setNewConnectionCallback(
		//Acceptor::handleRead函数中会回调用TcpServer::newConnection
       //_1对应得socket文件描述符，_2对应的是对等方的地址
            bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2)//绑定Acceptor::newConnectionCallback回调
            );
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    cout<<"TcpServer::~TcpServer [" << name_ << "] destructing"<<endl;

    for(auto& item:connections_){
        TcpConnectionPtr conn(item.second);
        item.second.reset();//释放TcpConnection对象
        conn->getLoop()->runInLoop(bind(&TcpConnection::connectDestroyed,conn));
    }
}

void TcpServer::setThreadNum(int numThreads) {//决定muduo库选择单线程模式还是多线程模式就是这一句话，//设置server中需要运行多少个Loop线程
    if(numThreads>=0)
        threadPool_->setThreadNum(numThreads);//设置I/O线程个数，不包含main Reactor
}

void TcpServer::start() {
    if(!started_){
        started_=true;
        threadPool_->start(threadInitCallback_);//启动线程池，创建numThreads_个线程和eventloop对象

        assert(!acceptor_->listenning());
		//因为acceptor是指针指征，库函数get_pointer可以返回原生指针
        loop_->runInLoop(
                bind(&Acceptor::listen,acceptor_.get())////Acceptor是在listen中开始关注可读事件
                );
    }

}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {//新连接处理函数
    loop_->assertInLoopThread();
    EventLoop* ioLoop=threadPool_->getNextLoop();//轮询调用线程池的EventLoop循环

    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);//端口+连接id
    ++nextConnId_;//++之后就是下一个连接id
    string connName = name_ + buf;//生成唯一的name

    cout<< "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort()<<endl;

    //获取本地地址
    InetAddress localAddr(socketOps::getLocalAddr(sockfd));//构造本地地址

    TcpConnectionPtr conn(new TcpConnection(ioLoop,
            connName,
            sockfd,
            localAddr,
            peerAddr
            ));//构造新的TcpConnection,将获取的EventLoop的地址传给新连接对象。

	//TcpConnection的use_count此处为1，新建了一个Tcpconnection
    connections_[connName]=conn;//将该TcpConnection加入到TcpServer的map容器中
	 //TcpConnection的use_count此处为2，因为加入到connections_中

	//设置TcpConnection三个半事件回调函数，将用户给TcpServer设置的回调传递给TCPConnection
  //实际TcpServer的connectionCallback等回调函数是对conn的回调函数的封装，所以在这里设置过去
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    //将TcpServer的removeConnection设置到了TcpConnection的关闭回调函数中
    conn->setCloseCallback(
            std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    //调用conn->connectEstablished()
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
	//调用TcpConenction:;connectEstablished函数内部会将use_count加一然后减一，此处仍为2
	//但是本函数介绍结束后conn对象会析构掉，所以引用计数为1，仅剩connections_列表中存活一个
}


void TcpServer::removeConnection(const TcpServer::TcpConnectionPtr &conn) {
    loop_->runInLoop(
            bind(&TcpServer::removeConnectionInLoop,this,conn)
            );
}

void TcpServer::removeConnectionInLoop(const TcpServer::TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();

//    cout << "TcpServer::removeConnectionInLoop [" << name_
//             << "] - connection " << conn->name()<<endl;
	// 根据conn的name，从map容器中删除，此时引用计数会减1。erase之前引用计数为2（由前面的shared_from_this()保证），所以执行完erase，引用计数变为1
    size_t n=connections_.erase(conn->name());//将该TcpConnection从map容器中删除
    (void) n;
    assert(n==1);

	// 然后调用conn->connectDestroyed
    EventLoop* ioLoop=conn->getLoop();

	//这里一定要用Eventloop::queueInLoop(),否则会出现对象生命期管理问题？？
  //这里用bind让TCPConnection的生命期长到调用connectionDestroy()的时刻
    ioLoop->queueInLoop(
            bind(&TcpConnection::connectDestroyed,conn)
            );
}