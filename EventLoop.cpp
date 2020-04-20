//
// Created by oil_you on 2019/10/31.
//

#include "EventLoop.h"
#include <sys/eventfd.h>
#include <iostream>
#include <assert.h>
#include <signal.h>


using namespace std;

//当前线程EventLoop对象指针
	//线程局部存储

__thread EventLoop* t_loopInThisThread = 0;

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
        // LOG_TRACE << "Ignore SIGPIPE";
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;



int creatEventfd(){
	//eventfd() 创建一个 eventfd 对象，可以由用户空间应用程序实现事件等待/通知机制，或由内核通知用户空间应用程序事件
//    EFD_CLOEXEC
//    文件被设置成 O_CLOEXEC，创建子进程 (fork) 时不继承父进程的文件描述符。
//    EFD_NONBLOCK
//    文件被设置成 O_NONBLOCK，执行 read / write 操作时，不会阻塞。
//    EFD_SEMAPHORE
//    提供类似信号量语义的 read 操作，简单说就是计数值 count 递减 1
    int evtfd=::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd<0){
        cout<<"Failed in eventfd"<<endl;
        exit(-1);//0表示正常退出，非0表示非正常退出
    }
    return evtfd;
}


EventLoop::EventLoop() //初始化事件循环
:looping_(false),
quit_(false),
eventHandling_(false),
callingPendingFuntors_(false),
threadId_(this_thread::get_id()),//赋值真实id
wakeupChannel_(new Channel(this,wakeupFd_)),//创建wakeupChannel通道，
poller_(new Poller(this)),//构造了一个实际的poller对象
wakeupFd_(creatEventfd()),
currentActiveChannel_(nullptr),
functorList_()
{
    if (t_loopInThisThread){
        cout<<"Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << threadId_;
    } else{
        t_loopInThisThread=this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));//设定wakeupChannel的回调函数，即EventLoop的handleRead函数
    wakeupChannel_->enableReading();//注册可读事件
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread=NULL;
    //quit_??
}

//事件循环，不能跨线程调用
//只能在创建该对象的线程中调用
void EventLoop::loop(){
    assert(!looping_);
	assertInLoopThread();//断言处于创建该对象的线程中
    looping_=true;
    while(!quit_){
        activeChannels_.clear();//删除事件列表所有元素
        poller_->poll(&activeChannels_);//通过poller获取就绪的channel，放到activeChannels_中,poller会将发生的事件类型填写到channel的revents_中，供Channel::handleEvent使用,调用相应的事件回调函数
        eventHandling_=true;//处理就绪事件
        for(Channel* pChannel:activeChannels_){
            currentActiveChannel_=pChannel;
			//调用channel的事件处理函数handleEvent,根据poller设置的发生的事件类型，调用相应的用户函数
            currentActiveChannel_->handleEvent();
        }
        currentActiveChannel_=NULL;//处理完了赋空
        eventHandling_=false;

		//I/O线程设计比较灵活，通过下面这个设计也能够进行计算任务，否则当I/O不是很繁忙的时候，这个I/O线程就一直处于阻塞状态。
	 //我们需要让它也能执行一些计算任务
        doPendingFuntors();
    }
    looping_=false;
}

void EventLoop::quit() {
    quit_=true;
    if(!isInLoopThread()){
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if(isInLoopThread())
        cb();
    else
        queueInLoop(std::move(cb));//右值引用
}

//将任务添加到队列当中，队列就是成员pendingFunctors_数组容器
void EventLoop::queueInLoop(EventLoop::Functor cb) {
    {
        lock_guard<mutex> lock(mutex_);
        functorList_.push_back(std::move(cb));
    }
    if(!isInLoopThread()||callingPendingFuntors_)
        wakeup();
}

void EventLoop::updateChannel(Channel *channel) {//添加更新事件分发器到map中
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)  //从map中移除事件分发器
{
    assert(channel->ownerLoop() == this);
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

//唤醒EventLoop
void EventLoop::wakeup() {
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof one);
    if(n!=sizeof(one))
        exit(-1);
}

//实际上是wakeFd_的读回调函数
//我们看到它从wakeupFd读取了值之后并未处理，因为我们只是把wakeupFd实际上是eventfd作为等待/通知机制实现，这里是为了当我们向EventLoop的queue中也就是
// pendingFunctors_这个容器数组加入任务时，通过eventfd通知I/O线程从poll状态退出来执行I/O计算任务。
void EventLoop::handleRead() {
    uint64_t one=1;
    ssize_t n=read(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one))
        exit(-1);
}

// 1. 不是简单的在临界区内依次调用functor，而是把回调列表swap到functors中，这一方面减小了
//临界区的长度，意味着不会阻塞其他线程的queueInLoop()，另一方面也避免了死锁(因为Functor可能再次调用queueInLoop)
// 2. 由于doPendingFunctors()调用的Functor可能再次调用queueInLoop(cb)，这时queueInLoop()就必须wakeup(),否则新增的cb可能就不能及时调用了
// 3. muduo没有反复执行doPendingFunctors()直到pendingFunctors为空，这是有意的，否则I/O线程可能陷入死循环，无法处理I/O事件
void EventLoop::doPendingFuntors() {
    vector<Functor> functors;
    callingPendingFuntors_=true;
    {
        lock_guard<mutex> lock(mutex_);
        functors.swap(functorList_);
    }

    for(const auto functor:functors){
        functor();
    }
    callingPendingFuntors_=false;
}



