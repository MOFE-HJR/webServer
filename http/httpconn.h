#ifndef _HTTPCONN_H_
#define _HTTPCONN_H_

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);

    ssize_t read(int* saveErrno);

    ssize_t write(int* saveErrno);

    void closeConn();

    int getFd() const;
    
    //处理任务
    bool process();

    //获取iovec的长度
    int getIovLen(); 

    //判断是否活跃
    bool isKeepAlive() const;

    static bool isET;//是否ET模式
    static const char* srcDir;//存储资源路径
    static std::atomic<int> userCount;//用户数量
    
private:
   
    int fd_;
    struct  sockaddr_in addr_;

    bool isClose_;
    
    int iovCnt_;// 2
    struct iovec iov_[2];//0存response格式，1存回发文件内容
    
    Buffer readBuff_; // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};


#endif //_HTTPCONN_H_