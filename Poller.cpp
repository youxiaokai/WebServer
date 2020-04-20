//
// Created by oil_you on 2019/10/31.
//
//这是epoll模式epoll_event事件的数据结构，其中data不仅可以保存fd，也可以保存一个void*类型的指针。
//typedef union epoll_data {
//               void    *ptr;
//               int      fd;
//               uint32_t u32;
//               uint64_t u64;
//           } epoll_data_t;
//           struct epoll_event {
//               uint32_t     events;    // Epoll events
//               epoll_data_t data;      //User data variable
//           };

#include "Poller.h"
#include <sys/epoll.h>
#include <iostream>
#include <assert.h>

using namespace std;

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

Poller::Poller(EventLoop * loop)
:ownerLoop_(loop),
epollfd_(epoll_create1(EPOLL_CLOEXEC)),//创建epollfd，使用带1的版本
events_(KinitEventListSize)//vector这样用时初始化kInitEventListSize个大小空间
{
    if(epollfd_<0){
        perror("epoll_create1");
        exit(-1);
    }
}

Poller::~Poller() {
    close(epollfd_);//关闭
}

void Poller::poll(ChannelList *activeChannels) {
    int timeoutMs=TIMEOUT;
    int numEvents=epoll_wait(epollfd_,&*events_.begin(),//使用epoll_wait()，等待事件返回,返回发生的事件数目
            static_cast<int>(events_.size()),timeoutMs);//返回的事件集合在events_数组中，数组中实际存放的成员个数是函数的返回值
    if(numEvents>0){
        fillActiveChannels(numEvents,activeChannels);//调用fillActiveChannels，传入numEvents也就是发生的事件数目
        if(numEvents==events_.size())
            events_.resize(2*events_.size()); //如果返回的事件数目等于当前事件数组大小，就分配2倍空间
    }

}

//把返回到的这么多个事件添加到activeChannels
void Poller::fillActiveChannels(int numEvents, Poller::ChannelList *activeChannels) const {
    assert(numEvents<=events_.size());//确定它的大小小于events_的大小，因为events_是预留的事件vector  
    for(int i=0;i<numEvents;++i){//挨个处理发生的numEvents个事件，epoll模式返回的events_数组中都是已经发生额事件，这有别于select和poll
		//这是epoll模式epoll_event事件的数据结构，其中data不仅可以保存fd，也可以保存一个void*类型的指针。
//typedef union epoll_data {
//               void    *ptr;
//               int      fd;
//               uint32_t u32;
//               uint64_t u64;
//           } epoll_data_t;
//           struct epoll_event {
//               uint32_t     events;    // Epoll events 
//               epoll_data_t data;      //User data variable 
//           };
        Channel* channel= static_cast<Channel*> (events_[i].data.ptr);
        int fd=channel->fd();
        auto it=channels_.find(fd);
        assert(it!=channels_.end());
        assert(it->second==channel);

        channel->set_revents(events_[i].events);  //把已发生的事件传给channel,写到通道当中
        activeChannels->push_back(channel); //并且push_back进activeChannels
    }

}

//这个函数被调用是因为channel->enableReading()被调用，再调用channel->update()，
//再eventloop->updateChannel()，再->epoll或poll的updateChannel被调用
void Poller::updateChannel(Channel *channel) {
    const int index=channel->index();//channel是本函数参数，获得channel的index，初始状态index是-1
    if(index==kNew||index==kDeleted){//index是在poll中是下标，在epoll中是三种状态，上面有三个常量
        int fd=channel->fd();
		// a new one, add with EPOLL_CTL_ADD
        if(index==kNew){
            assert(channels_.find(fd)==channels_.end());
            channels_[fd]=channel;
        }
        else // index == kDeleted??//已经被删除的
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);//注册新的fd到epfd中；
    }else{//index == kAdded //已经注册的过得fd
        int fd=channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);  //确保目前它存在

        if(channel->isNoneEvent()){ //如果什么也没关注，就直接干掉
            update(EPOLL_CTL_DEL,channel);//从epollfd_中删除一个fd
            channel->set_index(kDeleted);//删除之后设为deleted，表示已经删除，只是从内核事件表中删除，在channels_这个通道数组中并没有删除
        }
        else{
            update(EPOLL_CTL_MOD,channel);//修改已经注册的fd的监听事件//有关注，那就只是更新。更新成什么样子channel中会决定
        }
    }
}

void Poller::removeChannel(Channel *channel){
    int fd=channel->fd();
    assert(channels_.find(fd)!=channels_.end());
    assert(channels_[fd]==channel);
    assert(channel->isNoneEvent());

    int index=channel->index();
    assert(index==kDeleted||index==kAdded);
    size_t n=channels_.erase(fd);

    (void) n; //编译的时候，一般会有warning，告诉你有未经使用的变量n。这样写，然后编译器不会提示这种warning
    assert(n==1);

    if(index==kAdded){//如果还在epollfd_中
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

void Poller::update(int operation, Channel *channel) {
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.events=channel->events();
    event.data.ptr=channel;
    int fd=channel->fd();

	//第二个参数表示动作
	//EPOLL_CTL_ADD：       注册新的fd到epfd中；
	//	EPOLL_CTL_MOD：      修改已经注册的fd的监听事件；
	//	EPOLL_CTL_DEL：        从epfd中删除一个fd；

    if (epoll_ctl(epollfd_, operation, fd, &event) < 0)//使用epoll_ctl,
    {
        //cout << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
}
