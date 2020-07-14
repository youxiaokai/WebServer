#include <iostream>
#include <signal.h>
#include "EventLoop.h"
#include "InetAddress.h"
#include <sys/timerfd.h>
#include "Acceptor.h"
#include <stdio.h>
#include <cassert>
#include <arpa/inet.h>
#include "EventLoopThread.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpServer.h"
#include "EventLoop.h"
#include<sys/types.h>
 #include<sys/stat.h>
#include<fcntl.h> 
#include <sys/mman.h>

using namespace std;

//class IgnoreSigPipe{
//public:
//    IgnoreSigPipe(){
//        ::signal(SIGPIPE,SIG_IGN);
//    }
//};
//
//IgnoreSigPipe initObj;

extern char favicon[555];

bool benchmark =true;


//响应回调函数
void onRequest(const HttpRequest& req, HttpResponse* resp)
{
    std::cout << "Headers " << req.methodToString() << " " << req.path() << std::endl;
    if (!benchmark)  //如果为真，打印头部
    {
        const std::map<string, string>& headers = req.headers();
        for (const auto& header : headers)
        {
            std::cout << header.first << ": " << header.second << std::endl;
        }
    }
    //如果路径为根路径
 if (req.path() == "/")
    {
        resp->setStatusCode(HttpResponse::k200Ok); //状态码200
         resp->setStatusMessage("OK");//ok
         resp->setContentType("text/html");//html文本
         resp->addHeader("Server", "Oil_you");//增加头部

        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        FILE* fp = NULL;
        const char* filePath="/home/oil_you/桌面/我的简历.html";
        string responseBody;
        if((fp = fopen(filePath, "rb")) == NULL){
            perror("fopen file");
            return;
        }
        while(fread(buffer, sizeof(buffer), 1, fp) == 1)//也可以mmap内存映射优化，但不知为何我这种方式跑不通？？？
        {
            responseBody.append(buffer);
            memset(buffer, 0, sizeof(buffer));
        }
        if(feof(fp))
        {
            responseBody.append(buffer);
        }
        else
        {
            std::cout << "error fread" << std::endl;
        }
        fclose(fp);

        resp->setBody(responseBody);              
    }
    else if (req.path() == "/favicon.ico") //如果访问/favicon.ico路径，响应发送一张图片
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("image/png");
        resp->setBody(string(favicon, sizeof favicon));
    }
    else if (req.path() == "/hello")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        resp->addHeader("Server", "Muduo");
        resp->setBody("hello, world!\n");
    }
    else
    {
        resp->setStatusCode(HttpResponse::k404NotFound); //如果都不是，出现我们没有设置的路径，那么返回404，找不到
        resp->setStatusMessage("Not Found");
        resp->setCloseConnection(true);//断开连接
    }
}

int main(int argc, char* argv[])
{
    int numThreads = 0;
    if (argc > 1)
    {
        benchmark = false; //用来标志服务器是否打印header信息
        numThreads = atoi(argv[1]); //多线程数，如果为0，则不启动多线程
    }
    EventLoop loop;
    HttpServer server(&loop, InetAddress(12345), "dummy");//HttpServer类，客端可以以基于对象的方式把它包含起来
    server.setHttpCallback(onRequest);//设置响应回调
    server.setThreadNum(numThreads); //可能启动多线程
    server.start(); //启动
    loop.loop(); //循环等待
}

//这是一个图片数组，省略了一部分
char favicon[555] = {
        '\x89', 'P', 'N', 'G', '\xD', '\xA', '\x1A', '\xA',
        '\x0', '\x0', '\x0', '\xD', 'I', 'H', 'D', 'R',
        '\x0', '\x0', '\x0', '\x10', '\x0', '\x0', '\x0', '\x10',
        '\x8', '\x6', '\x0', '\x0', '\x0', '\x1F', '\xF3', '\xFF',
        'a', '\x0', '\x0', '\x0', '\x19', 't', 'E', 'X',
        't', 'S', 'o', 'f', 't', 'w', 'a', 'r',
        'e', '\x0', 'A', 'd', 'o', 'b', 'e', '\x20',
        'I', 'm', 'a', 'g', 'e', 'R', 'e', 'a',
        'd', 'y', 'q', '\xC9', 'e', '\x3C', '\x0', '\x0',
        '\x1', '\xCD', 'I', 'D', 'A', 'T', 'x', '\xDA',
        '\x94', '\x93', '9', 'H', '\x3', 'A', '\x14', '\x86',
        '\xFF', '\x5D', 'b', '\xA7', '\x4', 'R', '\xC4', 'm',
        '\x22', '\x1E', '\xA0', 'F', '\x24', '\x8', '\x16', '\x16',
        'v', '\xA', '6', '\xBA', 'J', '\x9A', '\x80', '\x8',
        'A', '\xB4', 'q', '\x85', 'X', '\x89', 'G', '\xB0',
        'I', '\xA9', 'Q', '\x24', '\xCD', '\xA6', '\x8', '\xA4',
        'H', 'c', '\x91', 'B', '\xB', '\xAF', 'V', '\xC1',
        'F', '\xB4', '\x15', '\xCF', '\x22', 'X', '\x98', '\xB',
        'T', 'H', '\x8A', 'd', '\x93', '\x8D', '\xFB', 'F',
        'g', '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f', 'v',
        'f', '\xDF', '\x7C', '\xEF', '\xE7', 'g', 'F', '\xA8',
        '\xD5', 'j', 'H', '\x24', '\x12', '\x2A', '\x0', '\x5',
        '\xBF', 'G', '\xD4', '\xEF', '\xF7', '\x2F', '6', '\xEC',
        '\x12', '\x20', '\x1E', '\x8F', '\xD7', '\xAA', '\xD5', '\xEA',
        '\xAF', 'I', '5', 'F', '\xAA', 'T', '\x5F', '\x9F',
        '\x22', 'A', '\x2A', '\x95', '\xA', '\x83', '\xE5', 'r',
        '9', 'd', '\xB3', 'Y', '\x96', '\x99', 'L', '\x6',
        '\xE9', 't', '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',
        '\xA7', '\xC4', 'b', '1', '\xB5', '\x5E', '\x0', '\x3',
        'h', '\x9A', '\xC6', '\x16', '\x82', '\x20', 'X', 'R',
        '\x14', 'E', '6', 'S', '\x94', '\xCB', 'e', 'x',
        '\xBD', '\x5E', '\xAA', 'U', 'T', '\x23', 'L', '\xC0',
        '\xE0', '\xE2', '\xC1', '\x8F', '\x0', '\x9E', '\xBC', '\x9',
        'A', '\x7C', '\x3E', '\x1F', '\x83', 'D', '\x22', '\x11',
        '\xD5', 'T', '\x40', '\x3F', '8', '\x80', 'w', '\xE5',
        '3', '\x7', '\xB8', '\x5C', '\x2E', 'H', '\x92', '\x4',
        '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g', '\x98',
        '\xE9', '6', '\x1A', '\xA6', 'g', '\x15', '\x4', '\xE3',
        '\xD7', '\xC8', '\xBD', '\x15', '\xE1', 'i', '\xB7', 'C',
        '\xAB', '\xEA', 'x', '\x2F', 'j', 'X', '\x92', '\xBB',
        '\x18', '\x20', '\x9F', '\xCF', '3', '\xC3', '\xB8', '\xE9',
        'N', '\xA7', '\xD3', 'l', 'J', '\x0', 'i', '6',
        '\x7C', '\x8E', '\xE1', '\xFE', 'V', '\x84', '\xE7', '\x3C',
        '\x9F', 'r', '\x2B', '\x3A', 'B', '\x7B', '7', 'f',
        'w', '\xAE', '\x8E', '\xE', '\xF3', '\xBD', 'R', '\xA9',
        'd', '\x2', 'B', '\xAF', '\x85', '2', 'f', 'F',
        '\xBA', '\xC', '\xD9', '\x9F', '\x1D', '\x9A', 'l', '\x22',
        '\xE6', '\xC7', '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15',
        '\x90', '\x7', '\x93', '\xA2', '\x28', '\xA0', 'S', 'j',
        '\xB1', '\xB8', '\xDF', '\x29', '5', 'C', '\xE', '\x3F',
        'X', '\xFC', '\x98', '\xDA', 'y', 'j', 'P', '\x40',
        '\x0', '\x87', '\xAE', '\x1B', '\x17', 'B', '\xB4', '\x3A',
        '\x3F', '\xBE', 'y', '\xC7', '\xA', '\x26', '\xB6', '\xEE',
        '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
        '\xA', '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X', '\x0',
        '\x27', '\xEB', 'n', 'V', 'p', '\xBC', '\xD6', '\xCB',
        '\xD6', 'G', '\xAB', '\x3D', 'l', '\x7D', '\xB8', '\xD2',
        '\xDD', '\xA0', '\x60', '\x83', '\xBA', '\xEF', '\x5F', '\xA4',
        '\xEA', '\xCC', '\x2', 'N', '\xAE', '\x5E', 'p', '\x1A',
        '\xEC', '\xB3', '\x40', '9', '\xAC', '\xFE', '\xF2', '\x91',
        '\x89', 'g', '\x91', '\x85', '\x21', '\xA8', '\x87', '\xB7',
        'X', '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N', 'N',
        'b', 't', '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
        '\xEC', '\x86', '\x2', 'H', '\x26', '\x93', '\xD0', 'u',
        '\x1D', '\x7F', '\x9', '2', '\x95', '\xBF', '\x1F', '\xDB',
        '\xD7', 'c', '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF',
        '\x22', 'J', '\xC3', '\x87', '\x0', '\x3', '\x0', 'K',
        '\xBB', '\xF8', '\xD6', '\x2A', 'v', '\x98', 'I', '\x0',
        '\x0', '\x0', '\x0', 'I', 'E', 'N', 'D', '\xAE',
        'B', '\x60', '\x82',
};

// if (req.path() == "/")
//     {
//         resp->setStatusCode(HttpResponse::k200Ok); //状态码200
//         resp->setStatusMessage("OK");//ok
//         resp->setContentType("application/pdf");//html文本
//         resp->addHeader("Server", "Oil_you");//增加头部

//          struct stat statbuf;
//          int srcFd;
//         const char* filePath="/home/oil_you/桌面/1.pdf";
     
//         if(srcFd=open( filePath,O_RDONLY,0)<0){
//             perror("open file");
//         }

//         if(fstat(srcFd,&statbuf)==-1){
//             perror("fstat");
//         }
//        void* mmapRet=::mmap(NULL,statbuf.st_size,PROT_READ,MAP_PRIVATE,srcFd,0);
//         close(srcFd);
//         char* srcAddr=static_cast<char*>(mmapRet);
//             if (MAP_FAILED == srcAddr){//若是mmap函数调用失败
//                  perror("mmap ");
//             }

//       string files(srcAddr,statbuf.st_size);
//         resp->setBody(files);  
        
//         if(munmap(srcAddr,statbuf.st_size)==-1){
//             perror("munmap error");
//           }
//     }