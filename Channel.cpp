//
// Created by oil_you on 2019/10/31.
//

#include "Channel.h"
#include <assert.h>
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0; //几种事件类型定义
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;  //PRI比如TCP的带外数据
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop * loop, int fd)
:loop_(loop),
fd_(fd),
events_(0),
revents_(0),
index_(-1),
eventHandling_(false),
addedToLoop_(false)
{
}

Channel::~Channel() {
//assert(!eventHandling_);
//assert(!addedToLoop_);
}

void Channel::handleEvent() {
    eventHandling_=true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))//发生挂起并且不可读
    {
        if (closeCallback_) closeCallback_();//调用关闭回调函数
    }

    if (revents_ & (EPOLLERR ))//发生错误、描述符不是一个打开的文件
    {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) //POLLIN | POLLPRI可读事件   //POLLRDHUP是对端关闭连接事件，如shutdown等
    {
        if (readCallback_) readCallback_();
    }
    if (revents_ & EPOLLOUT) //普通数据可写
    {
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;//处理完了=false

}

void Channel::update() {//更新事件类型
    addedToLoop_=true;
    loop_->updateChannel(this);//调用loop的，loop再调用poll的注册pollfd
}

void Channel::remove() {//移除channel
    addedToLoop_=false;
    loop_->removeChannel(this);//同上，先调用loop的removechannel，再调用poll的removechannel
}


