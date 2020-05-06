//
// Created by oil_you on 2019/11/11.
//

#include "HttpServer.h"
#include "HttpContext.h"
#include "TimerManager.h"

namespace detail{
void defaultHttpCallback(const HttpRequest&,HttpResponse* resp){
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}
}

HttpServer::HttpServer(EventLoop *loop,
        const InetAddress &listenAddr,
        const string &name,
        TcpServer::Option option)
        :server_(loop,listenAddr,name,option),
        httpCallback_(detail::defaultHttpCallback)
{
    //连接到来回调该函数
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection,this,std::placeholders::_1));
    //消息到来回调该函数
    server_.setMessageCallback(
            std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    TimerManager::GetTimerManagerInstance()->Start(); //HttpServer定时器管理器启动
}

void HttpServer::start() {
    std::cout << "HttpServer[" << server_.name()
             << "] starts listenning on " << server_.ipPort();
    server_.start();//TcpServer::start()，开始socket的listen
}

//该函数为一个新的TcpConnection绑定一个HttpContext对象，使用的是TcpConnection中的boost::any，
//绑定之后，HttpContext就相当于TcpConnection的成员了，
// TcpConection在MessageCallback中就可以随意的使用HttpContext对象了。
void HttpServer::onConnection(const TcpConnection::TcpConnectionPtr &conn) {
    if(conn->connected()){
        //构造一个http上下文对象，用来解析http请求
        conn->setContext(HttpContext());
        printf("onConnection():new connection [%s] form %s\n",
               conn->name().c_str(),
               conn->peerAddress().toIpPort().c_str());
        Timer* timer=new Timer(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::shutdown, conn));
        timer->Start();//添加timer到管理器
        conn->setTimer(timer);
    }
}

void HttpServer::onMessage(const TcpConnection::TcpConnectionPtr &conn, Buffer *buf) {
    //取出请求，mutable可以改变
    HttpContext* context=boost::any_cast<HttpContext>(conn->getMutableContext());
    Timer* timer= conn->getMutableTimer();

    timer->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::shutdown, conn));

    //调用context的parseRequest解析请求，返回bool是否请求成功
    if(!context->parseRequest(buf)){
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");//失败，发送400
        conn->shutdown(); //关闭连接
    }

//    printf("onMessage():received %zd byte from connection [%s]\n",buf->readableBytes(),
//           conn->name().c_str());

    if(context->kGotAll){//请求成功
        //调用onRequest
        onRequest(conn,context->request());
        //一旦请求处理完毕，重置context，因为HttpContext和TcpConnection绑定了，我们需要解绑重复使用。
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnection::TcpConnectionPtr & conn, const HttpRequest & req) {
    const string& connection=req.getHeader("Connection"); //取出头部
    bool close=(connection=="close"||
            (req.getVersion()==HttpRequest::kHttp10 && connection!="Keep-Alive"));

    //使用close构造一个HttpResponse对象，该对象可以通过方法.closeConnection()判断是否关闭连接
    HttpResponse response(close);
    httpCallback_(req,&response);//执行用户注册的回调函数
    Buffer buf;
    response.appendToBuffer(&buf);//用户处理后的信息，追加到缓冲区
    conn->send(&buf);//发送数据
    if(response.closeConnection()){//如果关闭
        conn->shutdown();//关了它
    }
}





