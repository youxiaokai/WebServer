//
// Created by oil_you on 2019/11/10.
//

#include "HttpResponse.h"

void HttpResponse::appendToBuffer(Buffer* output) const {
    char buf[32];
    snprintf(buf, sizeof(buf),"HTTP/1.1 %d ",statusCode_);
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    if(closeConnection_){
        output->append("Connection: close\r\n");//表示关闭该连接
    }
    else{
        snprintf(buf, sizeof(buf),"Content-Length: %zd\r\n",body_.size());//写入实体行
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n"); //表示保持该连接
    }

    for(const auto& header:headers_){
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body_);
}
