#ifndef _BUFFER_H_
#define _BUFFER_H_
#include <cstring>  //ssize_t size_t
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector>
#include <atomic>
#include <assert.h>
class Buffer {
public:
    
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    //返回可写数（写指针到缓冲区尾的长度）
    size_t writableBytes() const;
    //返回可读数（读指针到写指针的长度）
    size_t readableBytes() const;
    //返回开始写的位置
    char* beginWritePtr();
    //返回开始读的位置
    char* beginReadPtr();

    //移动读写指针
    void offWritePos(size_t len);
    void offReadPos(size_t len);
    void offReadPosByPtr(char* end);

    //清空缓冲区
    void cleanBuffer();
    //清空缓冲区并返回里面的数据
    std::string cleanBufGetStr();

    //根据fd读写数据
    ssize_t readFd(int fd , int* Errno);
    ssize_t writeFd(int fd , int* Errno);
    ssize_t writeFdByIov(int fd,struct iovec* iov,int iovCnt,int* err);

    //用string向缓冲区写入数据
    void appendByStr(const std::string& str);

    //测试
    void testBuf();

private:
    //缓冲区头指针
    char* beginPtr_();

    //写入超出缓冲区可写大小的数据
    void appendSpace_(const char* str , size_t len);
    //调整缓冲区空间使得可以写入数据
    void makeSpace_(size_t len);


    std::vector<char> buffer_;//缓冲区
    std::atomic<std::size_t> readPos_;//读指针位置
    std::atomic<std::size_t> writePos_;//写指针位置
};

#endif //_BUFFER_H_