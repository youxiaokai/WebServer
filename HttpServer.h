//
// Created by oil_you on 2019/11/11.
//

#ifndef CMAKE_BUILD_DEBUG_HTTPSERVER_HTTPSERVER_H
#define CMAKE_BUILD_DEBUG_HTTPSERVER_HTTPSERVER_H

#include "TcpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"



class HttpServer:boost::noncopyable {
public:
    typedef std::function<void(const HttpRequest&,
            HttpResponse*)> HttpCallback;

    HttpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& name,
            TcpServer::Option option=TcpServer::kNoReusePort);

    EventLoop* getLoop() const {return server_.getLoop();}

    //设置响应回调
    void setHttpCallback(const HttpCallback& cb){
        httpCallback_=cb;
    }
    //可能启动多线程
    void setThreadNum(int numThreads){
        server_.setThreadNum(numThreads);
    }

    void start();

private:
    void onConnection(const TcpConnection::TcpConnectionPtr& conn);
    void onMessage(const TcpConnection::TcpConnectionPtr& conn,
            Buffer* buf);
    void onRequest(const TcpConnection::TcpConnectionPtr&,const HttpRequest&);

    TcpServer server_;//http服务器也是一个Tcp服务器，所以包含一个TcpServer
    HttpCallback httpCallback_;//在处理http请求时(即调用onRequest)的过程中回调此函数，对请求进行具体的处理
};


#endif //CMAKE_BUILD_DEBUG_HTTPSERVER_HTTPSERVER_H
