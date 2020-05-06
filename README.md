# WebServer
A C ++ multi-thread server framework

## Introduction
本项目为C++11编写的基于epoll的多线程服务器框架，应用层实现了一个简单的http服务器，实现了http的head解析和get的请求，目前仅支持静态资源的访问，支持HTTP长连接。该框架可以用于实现其他应用服务，目前仅实现了HTTP服务器。

该项目参考了陈硕的muduo库和chenshuaihao的开源项目，再次表示感谢！

## Origin and purpose

- 项目起源：

  1.当时在学习网络编程时看的一本书叫UNP，里面介绍了各种简单的网络模型，觉得很有意思。但是又觉得这些太简单了，我就很好奇，真正的企业应用中服务器的网络模型又应该是什么样子的呢?于是想从底层一探究竟。

  2.因为从研一开始就确定好了将来找工作的方向是后台开发，但是因为实验室的项目都是和图像处理相关，于是自己便打算做个项目练练手。找来找去，发现很多都是什么坦克大战，2048小游戏，感觉太简单了......后来无意中发现陈硕有本讲服务器编程的书写的很好，于是决定写一个服务器的框架，一边写代码也一边学习吧。  

- 项目目的：

  简而言之，可以将后台相关的知识都贯穿起来。可以学习C++11语法及其新的特性，学习计算机网络、网络编程相关知识，还可以学习操作系统、linux命令、git相关操作等

## Envoirment

- OS：Ubuntu 18.04.4 LTS
- compiler：gcc  7.4.0

## Build

```
$cmake .
$make
```

## run

```
$ ./HttpServer [iothreadnum] 
例：$ ./HttpServer 4
表示采用4个IO线程的工作方式（默认端口为12345） 
```

## Technical points

- 使用 Epoll 水平触发（LT）的 IO 多路复用技术，非阻塞 IO，使用 Reactor 模式；
- 使用多线程充分利用多核 CPU，使用 Reactor pool 避免线程频繁创建销毁的开销；
- 主线程只负责accept 请求，并以 Round Robin 的方式分发给其它 IO 线程(兼计算线程)；
- 使用互斥锁和条件变量进行线程同步，锁的争用只会出现在主线程和某一特定线程中；
- 使用 eventfd 实现了线程的异步唤醒；
- 为减少内存泄漏的可能，采用智能指针管理多线程下的对象资源；
- 基于 vector 实现动态可增长的应用层缓冲区 Buffer；
- 支持HTTP长连接

## Concurrency model

采用multiReactor+非阻塞IO+epoll的模式，mainMeactor负责接受请求，subReactor负责处理处理具体的读写任务.

![并发模型](https://github.com/youxiaokai/WebServer/blob/master/model.png?raw=true)

## Performance Test

本项目采用了开源的压测工具ab进行了压测，在开启4个IO线程的情况下，压测结果如下：

```
ab -n 5000 -c 200 http://0.0.0.0:12345/ 

This is ApacheBench, Version 2.3 <$Revision: 1807734 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 0.0.0.0 (be patient)
Completed 500 requests
Completed 1000 requests
Completed 1500 requests
Completed 2000 requests
Completed 2500 requests
Completed 3000 requests
Completed 3500 requests
Completed 4000 requests
Completed 4500 requests
Completed 5000 requests
Finished 5000 requests


Server Software:        Muduo
Server Hostname:        0.0.0.0
Server Port:            12345

Document Path:          /
Document Length:        2124 bytes

Concurrency Level:      200
Time taken for tests:   0.201 seconds
Complete requests:      5000
Failed requests:        0
Total transferred:      11010000 bytes
HTML transferred:       10620000 bytes
Requests per second:    24848.67 [#/sec] (mean)
Time per request:       8.049 [ms] (mean)
Time per request:       0.040 [ms] (mean, across all concurrent requests)
Transfer rate:          53434.35 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.0      2       3
Processing:     2    4   1.5      4      12
Waiting:        1    4   1.6      3      12
Total:          3    5   1.1      5      12
WARNING: The median and mean for the initial connection time are not within a normal deviation
        These results are probably not that reliable.

Percentage of the requests served within a certain time (ms)
  50%      5
  66%      5
  75%      5
  80%      6
  90%      6
  95%      7
  98%     10
  99%     11
 100%     12 (longest request)
```

可以看到，该服务器运行稳定，吞吐量达到  24848.67 [#/sec] ，存在一定的瓶颈，还需要进一步完善提升。

## Development log

2020.4.20 第一次上传：实现多线程架构（IO线程池），主线程负责监听，accept连接后，通过以 Round Robin 的方式分发给其它 IO 线程实现负载均衡，IO线程负责连接的事件监听、读写操作。

2020.5.6  实现时间轮。基于时间轮实现定时器功能，定时剔除不活跃连接，添加、删除定时器的时间复杂度为O(1)，执行定时器任务的时间复杂度取决于每个桶（slot）上的链表长度。因为定时器被分散到各个桶上面，因此执行效率大大提升。

## Others

本项目将持续更新完善。由于时间关系，目前只完成了初步框架编写，性能存在一定瓶颈，还需进一步提升完善。

初步构想，接下来将添加时间轮、工作线程池、异步日志等功能。



