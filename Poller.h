//
// Created by oil_you on 2019/10/31.
//

#ifndef NETSERVER_POLLER_H
#define NETSERVER_POLLER_H

#include <boost/noncopyable.hpp>
#include <map>
#include "Channel.h"
#include "EventLoop.h"
#include "Channel.h"
#include <sys/epoll.h>

using namespace std;

//Class Channel;
class EventLoop;
class Channel;

class Poller:boost::noncopyable {
public:
    typedef vector<Channel*> ChannelList;

    Poller();
    Poller(EventLoop*);
    ~Poller();

    void poll(ChannelList* activeChannels);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    static const int KinitEventListSize=16; //默认事件数组大小，是用来装epoll_wait()返回的可读或可写事件的
    static const int TIMEOUT=1000;//epoll_wait 超时时间设置

    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;
    void update(int operation,Channel* channel);

    typedef map<int,Channel*> ChannelMap;
    typedef vector<struct  epoll_event> EventList;

    ChannelMap channels_;
    EventLoop* ownerLoop_;
    int epollfd_;
    EventList events_; //用来存发生事件的数组(活动的fd),大小为自适应的

};


#endif //NETSERVER_POLLER_H
