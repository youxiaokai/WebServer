//
// Created by oil_you on 2019/11/10.
//
// HTTP/1.1 200 OK
// Server: nginx/1.13.12
// Date: Fri, 15 Feb 2019 09:57:21 GMT
// Content-Type: text/html; charset=utf-8
// Transfer-Encoding: chunked
// Connection: keep-alive
// Vary: Accept-Encoding
// Vary: Cookie
// X-Frame-Options: SAMEORIGIN
// Set-Cookie: __bqusername=""; Domain=.bigquant.com; expires=Thu, 01-Jan-1970 00:00:00 GMT; Max-Age=0; Path=/
// Access-Control-Allow-Origin: *
// Content-Encoding: gzip

// 200：请求被正常处理
// 204：请求被受理但没有资源可以返回
// 206：客户端只是请求资源的一部分，服务器只对请求的部分资源执行GET方法，相应报文中通过Content-Range指定范围的资源。

// 301：永久性重定向
// 302：临时重定向
// 303：与302状态码有相似功能，只是它希望客户端在请求一个URI的时候，能通过GET方法重定向到另一个URI上
// 304：发送附带条件的请求时，条件不满足时返回，与重定向无关
// 307：临时重定向，与302类似，只是强制要求使用POST方法

// 400：请求报文语法有误，服务器无法识别
// 401：请求需要认证
// 403：请求的对应资源禁止被访问
// 404：服务器无法找到对应资源

// 500：服务器内部错误
// 503：服务器正忙

#ifndef CMAKE_BUILD_DEBUG_NETSERVER_HTTPRESPONSE_H
#define CMAKE_BUILD_DEBUG_NETSERVER_HTTPRESPONSE_H

#include <string>
#include <map>
#include "Buffer.h"

using std::string;
using std::map;

class HttpResponse {
public:
    enum HttpStatusCode{ //状态码
        kUnknown,
        k200Ok=200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
        K505VersionNotSurpported=505
    };

    explicit HttpResponse(bool close)
    :statusCode_(kUnknown),
    closeConnection_(close)
    {
    }

    void setStatusCode(HttpStatusCode code){statusCode_=code;} //设置状态码
    void setStatusMessage(const string& message){statusMessage_=message;} //设置状态信息

    void setCloseConnection(bool on){closeConnection_=on;} //设置是否关闭连接
    bool closeConnection() const{ return closeConnection_; }

    void setContentType(const string& contentType){//设置实体行中的文本类型
        addHeader("Content-Type",contentType);
    }

    void addHeader(const string& key,const string& value){ //添加头部行
        headers_[key]=value;
    }

    void setBody(const string& body){//设置实体行
        body_=body;
    }

    void appendToBuffer(Buffer* output) const;

private:
    map<string,string> headers_;
    HttpStatusCode statusCode_;

    string  statusMessage_;
    bool closeConnection_;//设置是否关闭连接
    string body_;
};


#endif //CMAKE_BUILD_DEBUG_NETSERVER_HTTPRESPONSE_H
