//
// Created by oil_you on 2019/11/10.
//

#ifndef CMAKE_BUILD_DEBUG_HTTPSERVER_HTTPCONTEXT_H
#define CMAKE_BUILD_DEBUG_HTTPSERVER_HTTPCONTEXT_H

#include "HttpRequest.h"
#include "Buffer.h"

class Buffer;

class HttpContext {
public:
    enum HttpRequestParseState//解析请求状态的枚举常量
    {
        kExpectRequestLine,//当前正处于解析请求行的状态
        kExpectHeaders, //当前正处于解析请求头部的状态
        kExpectBody, //当前正处于解析请求实体的状态
        kGotAll,  //解析完毕
    };

    HttpContext()
    :state_(kExpectRequestLine) //初始状态，期望收到一个请求行
    {
    }

    bool parseRequest(Buffer* buf);//解析请求报文

    bool gotAll() const{return state_==kGotAll;}

    //重置HttpContext状态，异常安全
    void reset(){
        state_=kExpectRequestLine;
        HttpRequest dummy;//构造一个临时空HttpRequest对象，和当前的成员HttpRequest对象交换置空，然后临时对象析构
        request_.swap(dummy);
    }

    const HttpRequest& request() const
    { return request_; }

    HttpRequest& request()
    { return request_; }




private:
    bool processRequestLine(const char* begin,const char* end);  //处理请求行

    HttpRequestParseState state_; //请求的解析状态
    HttpRequest request_;//http请求类

};


#endif //CMAKE_BUILD_DEBUG_HTTPSERVER_HTTPCONTEXT_H
