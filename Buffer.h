//
// Created by oil_you on 2019/11/11.
//

#ifndef CMAKE_BUILD_DEBUG_HTTPSERVER_BUFFER_H
#define CMAKE_BUILD_DEBUG_HTTPSERVER_BUFFER_H

#include <stddef.h>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <cstring>
#include <string>

using std::string;

class Buffer {
public:
    static const size_t kCheapPrepend = 8; //默认预留8个字节
    static const size_t kInitialSize = 102400000;  //初始大小216,046

    explicit  Buffer(size_t initialSize=kInitialSize)
    :buffer_(kCheapPrepend+initialSize),
    readerIndex_(kCheapPrepend),
    writerIndex_(kCheapPrepend)
    {
        //断言buffer的初始化各方面都符合逻辑
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    //默认复制构造函数、移动构造函数，构造函数

    void swap(Buffer& rhs){
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_,rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    //可读大小
    size_t readableBytes() const{return writerIndex_-readerIndex_;}
    //可写大小
    size_t writableBytes() const {return buffer_.size()-writerIndex_;}
    //预留大小
    size_t prependableBytes() const {return readerIndex_;}

    //开始读的位置
    const char* peek() const {
        return begin()+readerIndex_;
    }

    const char* findCRLF() const {
        const char* crlf=std::search(peek(),beginWrite(),kCRLF,kCRLF+2);
        return crlf==beginWrite()?NULL:crlf;
    }

    const char* findCRLF(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        // FIXME: replace with memmem()?
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findEOL() const
    {
        //C 库函数 void *memchr(const void *str, int c, size_t n)
        // 在参数 str 所指向的字符串的前 n 个字节中搜索第一次出现字符 c（一个无符号字符）的位置。
        const void* eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    const char* findEOL(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void* eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char*>(eol);
    }

    void retrieve(size_t len){
        assert(len<=readableBytes());
        if(len<readableBytes()){
            readerIndex_+=len;
        }
        else{
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveInt64()
    {
        retrieve(sizeof(int64_t));
    }

    void retrieveInt32()
    {
        retrieve(sizeof(int32_t));
    }

    void retrieveInt16()
    {
        retrieve(sizeof(int16_t));
    }

    void retrieveInt8()
    {
        retrieve(sizeof(int8_t));
    }

    string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        string result(peek(), len);
        retrieve(len);
        return result;
    }

    string toString() const
    {
        return string(peek(), static_cast<int>(readableBytes()));
    }

    void append(const string& str)
    {
        append(str.c_str(),str.size());
    }

    void append(const char* /*restrict*/ data, size_t len)
    {
        ensureWritableBytes(len);//确保缓冲区可写空间大于等于len，如果不足，需要扩充
        std::copy(data, data+len, beginWrite());//追加数据
        hasWritten(len);//内部仅仅是写入后调整writeindex
    }

    void append(const void* /*restrict*/ data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)//如果可写数据小于len
        {
            makeSpace(len); //增加空间
        }
        assert(writableBytes() >= len);
    }

    char* beginWrite()
    { return begin() + writerIndex_; }

    const char* beginWrite() const
    { return begin() + writerIndex_; }

    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    void unwrite(size_t len)//删除len长度的数据，实际就是移动writerIndex_
    {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    void prepend(const void* /*restrict*/ data, size_t len)//将data加入到prepend区域
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    //收缩空间，保留reserver个字节，可能多次读写后buffer太大了，可以收缩
    void shrink(size_t reserve)
    {
        // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
        Buffer other;//生成临时对像，保存readable内容，然后和自身交换，该临时对象再析构掉
        //ensureWritableBytes()函数有两个功能，一个是空间不够resize空间，一个是空间足够内部腾挪，这里明显用的是后者。
        other.ensureWritableBytes(readableBytes()+reserve);//确保有足够的空间，内部此时已经腾挪
        other.append(toString()); //把当前数据先追加到other里面，然后再交换。
        swap(other);//然后再交换
    }

    size_t internalCapacity() const//实际容量，预分配的内存空间
    {
        return buffer_.capacity();
    }

   //将数据直接读入缓冲区。
   //可以用readv（2）实现
   // @read（2）的返回结果， errno已保存
    ssize_t readFd(int fd, int* savedErrno);

private:
    char* begin(){return &(*buffer_.begin());}

    const char* begin() const
    { return &*buffer_.begin(); }

    void makeSpace(size_t len){
        if(writableBytes()+prependableBytes()<len+kCheapPrepend){//确保空间是真的不够，而不是挪动就可以腾出空间
            buffer_.resize(writerIndex_+len);
        }
        else{    //内部腾挪就足够append，那么就内部腾挪一下
            assert(kCheapPrepend<readerIndex_);
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,begin()+writerIndex_,
                    begin()+kCheapPrepend);//原来的可读部分全部copy到Prepend位置，相当于向前挪动，为writeable留出空间
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
            assert(readable=readableBytes());
        }
    }


    std::vector<char> buffer_; //vector用于替代固定数组
    size_t readerIndex_;  //读位置
    size_t writerIndex_;  //写位置

    static const char kCRLF[];   //'\r\n'，使用柔性数组

};


#endif //CMAKE_BUILD_DEBUG_HTTPSERVER_BUFFER_H
