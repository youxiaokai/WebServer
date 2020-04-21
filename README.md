# WebServer
A C ++ multi-thread server framework

## Introduction
本项目为C++11编写的基于epoll的多线程服务器框架，应用层实现了一个简单的http服务器，实现了http的head解析和get的请求，目前仅支持静态资源的访问，支持HTTP长连接。该框架可以用于实现其他应用服务，目前仅实现了HTTP服务器。

该项目参考了陈硕的muduo库和chenshuaihao的开源项目，再次表示感谢！

## Origin and purpose

- 项目起源：

  1.当时在学习网络编程时看的一本书叫UNP，里面介绍了各种简单的网络模型，觉得很有意思。但是又觉得这些太简单了，我就很好奇，真正的企业应用中服务器的网络模型又应该是什么样子的呢?于是想从底层一探究竟。

  2.因为从研一开始就确定好了将来找工作的方向是后台开发，但是因为实验室的项目都是和图像处理相关，于是自己便打算做个项目练练手。找来找去，发现很多都是什么坦克大战，2048小游戏......后来无意中发现陈硕有本讲服务器编程的书写的很好，于是决定写一个服务器的框架，一边写代码也一边学习吧。  

- 项目目的：

  简而言之，可以将后台相关的知识都贯穿起来。可以学习C++11语法及其新的特性，学习计算机网络、网络编程相关知识，还可以学习操作系统、linux命令、git相关操作等

## Envoirment

- OS：Ubuntu 18.04.4 LTS
- compiler：gcc  7.4.0

## run

```
$ ./main.out [iothreadnum] 
例：$ ./main.out 4
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

![并发模型](/home/oil_you/Project/Github/WebServer/model.png)











