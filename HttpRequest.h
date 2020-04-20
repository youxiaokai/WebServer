//
// Created by oil_you on 2019/11/10.
//
// GET /register.do?p={%22username%22:%20%2213917043329%22,%20%22nickname%22:%20%22balloon%22,%20%22password%22:%20%22123%22} HTTP/1.1\r\n
// GET / HTTP/1.1
// Host: baidu.com
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
// Accept-Encoding: gzip, deflate, br
// Accept-Language: zh-CN,zh;q=0.9,en;q=0.8
// Cookie: _bdid_=059a16ee3bef488b9d5212c81e2b688d; Hm_lvt_c58f67ca105d070ca7563b4b14210980=1550223017; _ga=GA1.2.265126182.1550223018; _gid=GA1.2.1797252688.1550223018; Hm_lpvt_c58f67ca105d070ca7563b4b14210980=1550223213; _gat_gtag_UA_124915922_1=1

#ifndef CMAKE_BUILD_DEBUG_NETSERVER_HTTPREQUEST_H
#define CMAKE_BUILD_DEBUG_NETSERVER_HTTPREQUEST_H

#include <string>
#include <map>
#include <assert.h>

using std::string;

class HttpRequest{
public:
    enum Method{  //请求方法
        kInvalid,kGet,kPost,kHead,kPut,kDelete
    };
    enum Version{
        kUnknown,kHttp10,kHttp11
    };

    HttpRequest()
    :method_(kInvalid),
    version_(kUnknown)
    {
    }


    void setVersion(Version v){ version_=v;}
    Version getVersion() const {return version_;}

    Method getMethod() const { return method_; }
    bool setMethod(const char* start,const char* end){
        assert(method_==kInvalid);
        //使用字符串首尾构造string，不包括尾部，如char *s="123", string s=(s,s+3),则s输出为123
        string m(start,end);
        if(m=="GET"){//请求指定的页面信息，并返回实体主体
            method_=kGet;
        }
        else if(m=="POST"){
            //向指定资源提交数据进行处理请求（例如提交表单或者上传文件）。
            // 数据被包含在请求体中。POST请求可能会导致新的资源的建立和/或已有资源的修改
            method_=kPost;
        }
        else if (m == "HEAD")//类似于get请求，只不过返回的响应中没有具体的内容，用于获取报头
        {
            method_ = kHead;
        }
        else if (m == "PUT")//从客户端向服务器传送的数据取代指定的文档的内容。
        {
            method_ = kPut;
        }
        else if (m == "DELETE")//请求服务器删除指定的页面。
        {
            method_ = kDelete;
        }
        else
        {
            method_ = kInvalid;
        }
        return method_ != kInvalid;
    }

    const char* methodToString() const
    {
        const char* result="UNKNOWN";
        switch(method_){
            case kGet:
                result="GET";
                break;
            case kPost:
                result="POST";
                break;
            case kHead:
                result = "HEAD";
                break;
            case kPut:
                result = "PUT";
                break;
            case kDelete:
                result = "DELETE";
                break;
            default:
                break;
        }
        return result;
    }

    void setPath(const char* start,const char* end){
        path_.assign(start,end);//assign方法可以理解为先将原字符串清空，然后赋予新的值作替换。
                                //返回类型为 string类型的引用
    }

    const string& path() const{
        return path_;
    }
    //设置参数
    void setQuery(const char* start, const char* end)
    {
        query_.assign(start, end);
    }

//    void setReceiveTime(Timestamp t)
//    { receiveTime_ = t; }
//
//    Timestamp receiveTime() const
//    { return receiveTime_; }

    //添加头部信息，将信息存储在map中
    void addHeader(const char* start,const char* colon,const char* end){//colon-冒号
        string field(start,colon);//header域
        ++colon;

        while(colon<end&&isspace(*colon)){
            ++colon;
        }
        string value(colon,end);//heade值
        //去除右空格，如果右边有空格会一直resize-1
        while(!value.empty()&&isspace(value[value.size()-1])){
            value.resize(value.size()-1);
        }

        headers_[field]=value;
    }

    //根据头部行的域返回值
    string getHeader(const string& field) const{
        string result;
        std::map<string,string>::const_iterator it=headers_.find(field);
        if(it!=headers_.end()){
            result=it->second;
        }
        return result;
    }

    const std::map<string,string>& headers() const{return headers_;}

    void swap(HttpRequest& that){
        std::swap(method_,that.method_);
        std::swap(version_,that.version_);
        std::swap(path_,that.path_);
        std::swap(query_,that.query_);
        std::swap(headers_,that.headers_);
    }

private:
    Method method_;
    Version version_;
    string path_;
    string query_;
    //Timestamp receiveTime_; //请求接收时间
    std::map<string,string> headers_;

};

#endif //CMAKE_BUILD_DEBUG_NETSERVER_HTTPREQUEST_H
