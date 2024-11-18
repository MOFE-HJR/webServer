#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>
#include <iostream>

#include "../epoller/epoller.h"
#include "../heaptimer/heaptimer.h"
#include "../connectpool/connectpool.h"
#include "../threadpool/threadpool.h"
#include "../http/httpconn.h"

class WebServer {
public:
    //四个提交的任务类
    friend class dealListen;
    friend class dealClose;
    friend class dealRead;
    friend class dealWrite;

    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger);

    ~WebServer();
    void start();

private:
    bool initSocket_(); 
    void initEventMode_(int trigMode);
    void addClient_(int fd , sockaddr_in addr , WebServer* web);

    void sendError_(int fd, const char*info);

    //延长连接时间
    void extentTime_(HttpConn* client);
    
    //关闭连接
    void closeConn_(HttpConn* client);

    //读客户端的信息
    void onRead_(HttpConn* client);
    //回发客户端
    void onWrite_(HttpConn* client);
    //处理客户端信息
    void onProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    static int setFdNonblock(int fd);

    int port_;
    bool openLinger_;//是否优雅关闭
    int timeoutMS_;//连接超时时长
    bool isClose_;
    int listenFd_;
    char* srcDir_;
    
    uint32_t listenEvent_;
    uint32_t connEvent_;
   
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool_> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;//存储cfd和http连接

};


#endif //_WEBSERVER_H_