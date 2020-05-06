cmake_minimum_required(VERSION 3.15)
project(cmake-build-debug/HttpServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(HttpServer main.cpp EventLoop.h EventLoop.cpp Poller.cpp Poller.h Channel.cpp Channel.h Socket.cpp Socket.h InetAddress.cpp InetAddress.h Acceptor.cpp Acceptor.h TcpServer.cpp TcpServer.h EventLoopThread.cpp EventLoopThread.h EventLoopThreadPool.cpp EventLoopThreadPool.h TcpConnection.cpp TcpConnection.h HttpRequest.h HttpResponse.cpp HttpResponse.h HttpContext.cpp HttpContext.h Buffer.cpp Buffer.h HttpServer.cpp HttpServer.h Timer.cpp Timer.h TimerManager.cpp TimerManager.h)

target_link_libraries(HttpServer pthread rt)