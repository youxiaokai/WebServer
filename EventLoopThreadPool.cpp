//
// Created by oil_you on 2019/11/6.
//

#include "EventLoopThreadPool.h"
#include <assert.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const string &name)
:baseLoop_(baseLoop),
name_(name),
started_(false),
numThreads_(0),
next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool() {
    //不要删除循环，它的堆栈是可变的
}

void EventLoopThreadPool::start(const EventLoopThreadPool::ThreadInitCallback &cb) {
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_=true;

    for(int i=0;i<numThreads_;++i){
        char buf[name_.size()+32];
        //在c语言中没有string类型，故必须通过string类对象的成员函数c_str()把string 对象转换成c中的字符串样式。
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        EventLoopThread* t=new EventLoopThread(cb,buf);
        threads_.push_back(unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());//每个EventLoop开始loop()
    }

    //线程池的个数为0，则主线程开始监听套接字
    if(numThreads_==0&& cb){
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop=baseLoop_;

    if(!loops_.empty()){
        loop=loops_[next_];
        ++next_;
        if(next_>=loops_.size()){
            next_=0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;

    if (!loops_.empty())
    {
        loop = loops_[hashCode % loops_.size()];
    }
    return loop;
}

vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    assert(!started_);
    baseLoop_->assertInLoopThread();

    if(loops_.empty()){
        return vector<EventLoop*>(1,baseLoop_);
    } else{
        return loops_;
    }
}
