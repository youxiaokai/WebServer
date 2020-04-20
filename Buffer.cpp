//
// Created by oil_you on 2019/11/11.
//

#include "Buffer.h"
#include <sys/uio.h>

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

//结合栈上空间，避免内存使用过大，提高内存使用率
//如果有10K个连接，每个连接就分配64K缓冲区的话，将占用640M内存
//而大多数时候，这些缓冲区的使用率很低
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    //节省一次ioctl系统调用(获取当前有多少可读数据）
    //为什么这么说?因为我们准备了足够大的extrabuf，那么我们就不需要使用ioctl取查看fd有多少可读字节数了
    char extrabuf[65536];

    //使用iovec分配两个连续的缓冲区
    struct iovec vec[2];
    const size_t writable = writableBytes();
    //第一块缓冲区，指向可写空间
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    //第二块缓冲区，指向栈上空间
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;//writeable一般小于65536
    const ssize_t n = readv(fd, vec, iovcnt);//iovcnt=2
    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (n <= writable)//第一块缓冲区足够容纳
    {
        writerIndex_ += n;//直接加n
    }
    else //当前缓冲区，不够容纳，因而数据被接受到了第二块缓冲区extrabuf，将其append至buffer
    {
        writerIndex_ = buffer_.size();//先更新当前writerIndex
        append(extrabuf, n - writable);//然后追加剩余的再进入buffer当中
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_30;
    // }
    return n;
}