//
// Created by oil_you on 2019/10/31.
//

#ifndef NETSERVER_CHANNEL_H
#define NETSERVER_CHANNEL_H

#include <boost/noncopyable.hpp>
#include <functional>
#include "EventLoop.h"
#include <iostream>


using namespace std;

class EventLoop;

//此类不拥有文件描述符。 ///文件描述符可以是一个套接字，一个eventfd，timerfd或signalfd
class Channel:boost::noncopyable{
public:
    typedef function<void()> EventCallback;//事件回调处理

    Channel(EventLoop*,int fd);//一个Channel一个EventLoop
    ~Channel();

    void handleEvent();//处理事件

	 //设置各种回调函数
    void setReadCallback(EventCallback cb){
        readCallback_=cb;
    }

    void setWriteCallback(EventCallback cb){
        writeCallback_=cb;
    }

    void setCloseCallback(EventCallback cb){
        closeCallback_=cb;
    }

    void setErroCallback(EventCallback cb){
        errorCallback_=cb;
    }

    int fd() const {
        return fd_;
    }

    int events()const {
        return events_;
    }

    void set_revents(int revt){revents_=revt;}//??需检查

    int index() const{return index_;}//pollfd数组中的下标
    void set_index(int idx){index_=idx;}

    bool isNoneEvent() const { return events_ == kNoneEvent; } //判断是否无关注事件类型，kNoneEvent为0

    void enableReading() { events_ |= kReadEvent; update();  }  //或上事件，就是关注可读事件，注册到EventLoop，通过它注册到Poller中
    void disableReading() { events_ &= ~kReadEvent; update(); }  //取消关注
    void enableWriting() { events_ |= kWriteEvent; update(); } //关注可写事件
    void disableWriting() { events_ &= ~kWriteEvent; update(); }  //关闭
    void disableAll() { events_ = kNoneEvent; update(); }   //关闭全部event？？
    bool isWriting() const { return events_ & kWriteEvent; }  //是否关注了写
    bool isReading() const { return events_ & kReadEvent; }  //是否关注读

    EventLoop* ownerLoop(){
        return loop_;
    }
    void remove();  //移除，确保调用前调用disableall

private:
    void update();

    static const int kNoneEvent;  //三个常量，没有事件，static常量类外部有定义，在.cc文件中
    static const int kReadEvent;  //可读事件
    static const int kWriteEvent;   //可写事件

    EventLoop* loop_;//记录所属EventLoop
    const int fd_;//文件描述符，但不负责关闭该描述符
    int events_;//关注的事件类型
    int revents_;//这是收到的epoll或poll事件类型
    int index_;// used by Poller.  表示在Poller事件数组中的序号

    bool eventHandling_;   //是否处于处理事件中
    bool addedToLoop_;//是否讲该Channel添加到EventLoop中

    EventCallback readCallback_;  //几种事件处理回调
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};


#endif //NETSERVER_CHANNEL_H
