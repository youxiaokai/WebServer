//
// Created by oil_you on 2019/11/6.
//

#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(const EventLoopThread::ThreadInitCallback &cb,const string& name)
:loop_(NULL),//loop未启动为NULL
exiting_(false),
thread_(),
threadName_(name),//绑定线程运行函数
callback_(cb)
{

}

EventLoopThread::~EventLoopThread() {
    exiting_=true;
    std::cout << "Clean up the EventLoopThread id: " << std::this_thread::get_id() << std::endl;
    if(loop_){
        loop_->quit();//退出I/O线程，让I/O线程的loop循环退出，从而退出了I/O线程
        thread_.join();
    }
}

EventLoop* EventLoopThread::getLoop() {
    return loop_;
}

EventLoop* EventLoopThread::startLoop() {//启动EventLoopThread中的loop循环，内部实际调用thread_.start
    thread_=std::thread(&::EventLoopThread::threadFunc,this);

    EventLoop* loop=NULL;
    {
        unique_lock<mutex> lock(mutex_);
        while(loop_==NULL){ // 如果标志位不为 true, 则等待...
            cond_.wait(lock);//让线程休眠
        }
        //主线程被唤醒
//        std::cout << "thread:";
//        printf("%d", this_thread::get_id());
//        cout<<" wakeup"<< '\n';
        loop=loop_;
    }
    return loop;
}

//该函数是EventLoopThread类的核心函数，作用是启动loop循环
//该函数和上面的startLoop函数并发执行，所以需要上锁和condition
void EventLoopThread::threadFunc() {
    EventLoop loop;

//    cout<<"In the thread:";
//    printf("%d\n", this_thread::get_id());

    if(callback_){
        callback_(&loop);//构造函数传递进来的，线程启动执行回调函数
    }

    {
        unique_lock<mutex> lock(mutex_);
        loop_=&loop;//然后loop_指针指向了这个创建的栈上的对象，threadFunc退出之后，这个指针就失效了
        cond_.notify_all();
    }

    loop.loop();//该函数退出，意味着线程就退出了，EventLoopThread对象也就没有存在的价值了。但是muduo的EventLoopThread
      //实现为自动销毁的。一般loop函数退出整个程序就退出了，因而不会有什么大的问题，
      //因为muduo库的线程池就是启动时分配，并没有释放。所以线程结束一般来说就是整个程序结束了。

    lock_guard<mutex> lock(mutex_);
    loop_=NULL;
}