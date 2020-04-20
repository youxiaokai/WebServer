//
// Created by oil_you on 2019/11/6.
//

#ifndef NETSERVER_EVENTLOOPTHREADPOOL_H
#define NETSERVER_EVENTLOOPTHREADPOOL_H


#include "EventLoop.h"
#include "EventLoopThread.h"
#include <vector>

using namespace std;

class EventLoopThreadPool {
public:
    typedef  function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseLoop,const string& name);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_=numThreads;}
    void start(const ThreadInitCallback& cb=ThreadInitCallback()); //启动线程池

    //新建连接，返回一个新的eventloop
    EventLoop* getNextLoop();
    //使用相同的hash code，它将始终返回相同的EventLoop
    EventLoop* getLoopForHash(size_t hashCode);

    vector<EventLoop*> getAllLoops();

    bool started() const
    { return started_; }

    const string& name() const
    { return name_; }

private:
    EventLoop* baseLoop_;//主Eventloop
    string name_;
    bool started_;
    int numThreads_; //线程池的大小
    int next_;    //用于轮询分发的索引
    vector<unique_ptr<EventLoopThread>> threads_; //线程列表
    vector<EventLoop*> loops_; //EventLoop列表

};


#endif //NETSERVER_EVENTLOOPTHREADPOOL_H
