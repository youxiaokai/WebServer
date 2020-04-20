//
// Created by oil_you on 2019/11/2.
//此文件为测试文件，对应书本代码P289

#include <iostream>
#include <signal.h>
#include "EventLoop.h"
#include <sys/timerfd.h>

using namespace std;

class IgnoreSigPipe{
public:
    IgnoreSigPipe(){
        ::signal(SIGPIPE,SIG_IGN);
    }
};

IgnoreSigPipe initObj;

EventLoop* g_loop;
int timerfd;

void timeout(){
    cout<<"Timeout!\n"<<endl;
    uint64_t one;
    ssize_t n=read(timerfd,&one,sizeof(one));
    if(n!=sizeof(one))
        exit(-1);
    //g_loop->quit();
}

int main() {
    EventLoop loop;
    g_loop=&loop;

    timerfd=::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);
    Channel channel(&loop,timerfd);
    channel.setReadCallback(timeout);
    channel.enableReading();

    struct itimerspec howlong;
    bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec=5;
    howlong.it_interval.tv_sec=1;
    ::timerfd_settime(timerfd,0,&howlong,NULL);

    loop.loop();

    close(timerfd);
}

