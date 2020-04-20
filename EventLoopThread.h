//
// Created by oil_you on 2019/11/6.
//

#ifndef NETSERVER_EVENTLOOPTHREAD_H
#define NETSERVER_EVENTLOOPTHREAD_H


#include <boost/core/noncopyable.hpp>
#include <functional>
#include "EventLoop.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <pthread.h>

using namespace std;

class EventLoop;

class EventLoopThread: boost::noncopyable {
public:
    typedef function<void(EventLoop*)> ThreadInitCallback;

    //右值引用，在对象返回的时候不会拷贝构造临时对象，而是和临时对象交换，提高了效率
    EventLoopThread(const ThreadInitCallback& cb=ThreadInitCallback(),const string& name=string());

    ~EventLoopThread();
    EventLoop* getLoop();
    EventLoop* startLoop();//启动成员thread_线程，该线程就成了I/O线程，内部调用thread_.start()

private:
    void threadFunc();//线程运行函数

    EventLoop* loop_; //线程运行的loop循环
    bool exiting_;
    thread thread_;   //线程成员
    string threadName_;

    mutex mutex_; // 全局互斥锁
    condition_variable cond_; // 全局条件变量.

    ThreadInitCallback callback_;//回调函数在EventLoop::loop事件循环之前被调用

};


#endif //NETSERVER_EVENTLOOPTHREAD_H
