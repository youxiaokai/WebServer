//
// Created by oil_you on 2019/10/31.
//

#ifndef NETSERVER_EVENTLOOP_H
#define NETSERVER_EVENTLOOP_H

#include <sched.h>
#include <memory>
#include <functional>
#include <boost/any.hpp>
#include <vector>
#include <mutex>
#include <thread>

#include "Channel.h"
#include "Poller.h"

using namespace std;

class Channel;
class Poller;

class EventLoop {
public:
    typedef function<void()> Functor;
    typedef vector<Channel*> ChannelList;

    EventLoop();
    ~EventLoop();

    void loop(); // 主循环
    void quit(); // 退出主循环
	///立即在循环线程中运行回调。
	///它唤醒循环，并运行cb。
	///如果在同一个循环线程中，则cb在该函数内运行。
	///从其他线程安全调用。
    void runInLoop(Functor cb);
	/// 插入主循环任务队列
    void queueInLoop(Functor cb);

	//唤醒事件通知描述符
    void wakeup();
    void updateChannel(Channel* channel);//添加某个事件分发器
    void removeChannel(Channel* channel);//移除某个事件分发器

    bool isInLoopThread()const {return threadId_==this_thread::get_id();}//检测是否在I/O线程中
    void assertInLoopThread()    //如果不在I/O线程中则退出程序
    {
        if (!isInLoopThread())
        {
            perror("EventLoop::abortNotInLoopThread!");
            exit(-1);
        }
    }

    void setContext(const boost::any& context){
        context_=context;
    };
    const boost::any& getContext(){
        return context_;
    };


private:
    void handleRead();// waked up //将事件通知描述符里的内容读走,以便让其继续检测事件通知？？
    void doPendingFuntors();//执行转交给I/O的任务

    bool looping_;/* atomic */   //是否运行
    bool quit_;//是否退出事件循环
    bool eventHandling_;/* atomic */  //是否正在处理事件
    bool callingPendingFuntors_;//是否执行doPendingFunctors()
    const thread::id threadId_;//当前所属对象线程id ，pid_t——进程ID
    unique_ptr<Poller> poller_; //poller对象
    int wakeupFd_; //唤醒套接字，用于线程间通信

    unique_ptr<Channel> wakeupChannel_;//wakeupfd所对应的通道，该通道会纳入到poller来管理
    boost::any context_;//boost::any能够保存任意类型的变量，这和STL有很大的不同；

    ChannelList activeChannels_;//Poller返回的活动通道，vector<channel*>类型
    Channel* currentActiveChannel_; //当前正在处理的活动通道

    mutex mutex_;
    vector<Functor> functorList_; //本线程或其它线程使用queueInLoop添加的任务，可能是I/O计算任务
};


#endif //NETSERVER_EVENTLOOP_H
