//
// Created by oil_you on 2019/11/10.
//

#include "HttpContext.h"

bool HttpContext::processRequestLine(const char *begin, const char *end) {
    bool succeed=false;
    const char* start=begin;
    const char* space=std::find(start,end,' ');//查找空格

    //找到GET并设置请求方法
    if(space!=end&& request_.setMethod(start,space)){
        start=space+1;
        space=std::find(start,end,' ');//再次查找
        if(space!=end){
            const char* question=std::find(start,space,'?');
            if(question!=space){//找到了'?'，说明有请求参数
                //设置路径
                request_.setPath(start,question);
                //设置请求参数
                request_.setQuery(question,space);
            }
            else{
                //没有找到只设置路径
                request_.setPath(start,space);
            }
            start=space+1;
            //查找有没有"HTTP/1."
            succeed=end-start==8&&std::equal(start,end-1,"HTTP/1.");
            if(succeed){//如果成功，判断是采用HTTP/1.1还是HTTP/1.0
                if(*(end-1)=='1'){
                    request_.setVersion(HttpRequest::kHttp11);
                }
                else if(*(end-1)=='0'){
                    request_.setVersion(HttpRequest::kHttp10);
                }
                else{
                    succeed=false;//请求行失败
                }
            }
        }
    }
    return succeed;
}

//解析请求
bool HttpContext::parseRequest(Buffer *buf) {
    bool ok=true;
    bool hasMore=true;
    while(hasMore){
        //初始状态是处于解析请求行的状态，下一次循环不是该状态就不会进入，一般只进入一次
        if(state_==kExpectRequestLine){
            //查找\r\n，返回其位置或者NULL
            //首先查找\r\n，就会到GET / HTTP/1.1的请求行末尾
            const char* crlf=buf->findCRLF();
            if(crlf){
                ok=processRequestLine(buf->peek(),crlf);//解析请求行
                if(ok){//如果成功，设置请求行事件
                    //将请求行从buf中取回，包括\r\n（光标移动到下一行）
                    buf->retrieveUntil(crlf+2);
                    state_=kExpectHeaders;
                }
                else
                {
                    hasMore=false;
                }
            }
            else{
                hasMore=false;
            }
        }
        else if(state_==kExpectHeaders){//处于Header状态
            const char* crlf=buf->findCRLF();
            if(crlf){
                const char* colon=std::find(buf->peek(),crlf,':');//查找:
                if(colon!=crlf){
                    request_.addHeader(buf->peek(),colon,crlf);//找到添加头部，加到map容器
                }
                else{
                    state_=kGotAll;//一旦请求完毕，再也找不到':'了，状态改为gotall状态，循环退出
                    hasMore= false;
                }
                buf->retrieveUntil(crlf+2);//请求完毕也把crlf取回,相当于读取光标移动到下一行
            }
            else{
                hasMore= false;
            }

        }
        else if(state_==kExpectBody){
            //
        }
    }
    return ok;
}
