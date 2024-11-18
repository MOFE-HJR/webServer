#include "webserver.h"

class dealListen :public Task_{
public:
    dealListen(WebServer* web) :web_(web){}
    Any_ run(){
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        do {
            int fd = accept(web_->listenFd_ , (struct sockaddr*)&addr , &len);
            if( fd <= 0 ) {
                return "";
            } else if( HttpConn::userCount >= web_->MAX_FD ) {
                web_->sendError_(fd , "Server busy!");
                return "";
            }
            //cfd addr_in_c
            web_->addClient_(fd , addr , web_);
        } while( web_->listenEvent_ & EPOLLET );
        return "";
    }

private:
    WebServer* web_;
};


class dealClose :public Task_{
public:
    dealClose(WebServer* web , HttpConn* client)
        : web_(web)
        , client_(client){}
    
    Any_ run(){
        web_->closeConn_(client_);
        return "";
    }

private:
    WebServer* web_;
    HttpConn* client_;
};


class dealRead :public Task_{
public:
    dealRead(WebServer* web , HttpConn* client)
        : web_(web)
        , client_(client){}

    Any_ run(){
        web_->extentTime_(client_);
        web_->onRead_(client_);
        return "";
    }

private:
    WebServer* web_;
    HttpConn* client_;
};


class dealWrite :public Task_{
public:
    dealWrite(WebServer* web , HttpConn* client)
        : web_(web)
        , client_(client){}

    Any_ run(){
        web_->extentTime_(client_);
        web_->onWrite_(client_);
        return "";
    }

private:
    WebServer* web_;
    HttpConn* client_;
};



using namespace std;

WebServer::WebServer(
            int port, int trigMode, int timeoutMS, bool OptLinger):
            port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
            timer_(new HeapTimer()), epoller_(new Epoller()){
    //获取绝对路径
    //第一个参数为空指针，让系统动态调用malloc分配内存，自己手动free
    srcDir_ = getcwd(nullptr, 256);
    if(srcDir_ == nullptr) std::cout<<"false"<<std::endl;
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);

    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    initEventMode_(trigMode);
    if(!initSocket_()) { isClose_ = true;}
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    //EPOLLONESHOT: 一个socket只能被一个线程持有
    //EPOLLRDHUP: close会发出EPOLLRDHUP 减少一次系统调用
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::start() {
    //epoll wait timeout == -1 无事件将阻塞 
    int timeMS = -1;  
    ThreadPool_ pool;
    pool.setMode(PoolMode_::MODE_CACHED);
    pool.start();
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = timer_->getNextTick();
        }
        int eventCnt = epoller_->wait(timeMS);
        for(int i = 0; i < eventCnt; i++) {
            //fd == epfd
            int fd = epoller_->getEventFd(i);
            //events == events[i].events
            uint32_t events = epoller_->getEvents(i);
            if(fd == listenFd_) {
                pool.submitTask(std::make_shared<dealListen>(this));
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                pool.submitTask(std::make_shared<dealClose>(this , &users_[fd]));
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                pool.submitTask(std::make_shared<dealRead>(this , &users_[fd]));
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                pool.submitTask(std::make_shared<dealWrite>(this , &users_[fd]));
            }
        }
    }
}

void WebServer::sendError_(int fd, const char*info) {
    assert(fd > 0);
    send(fd, info, strlen(info), 0);
    close(fd);
}

void WebServer::closeConn_(HttpConn* client) {
    assert(client);
    epoller_->delFd(client->getFd());
    client->closeConn();
}

void WebServer::addClient_(int fd , sockaddr_in addr , WebServer* web) {
    assert(fd > 0);
    //插入空key
    users_[fd].init(fd , addr);
    if( timeoutMS_ > 0 ) {
        //cfd
        timer_->add(fd , timeoutMS_ , std::bind(&WebServer::closeConn_ , web , &(web->users_[fd])));
    }
    epoller_->addFd(fd , EPOLLIN | connEvent_);
    setFdNonblock(fd);
}

void WebServer::extentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { timer_->adjust(client->getFd(), timeoutMS_); }
}

void WebServer::onRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        closeConn_(client);
        return;
    }
    onProcess(client);
}

void WebServer::onProcess(HttpConn* client) {
    //if 读取到了客户端的请求 (回发资源)EPOLLOUT
    if(client->process()) {
        epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->modFd(client->getFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->getIovLen() == 0 ) {
        //传输完成
        if(client->isKeepAlive()) {
            onProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            //继续
            epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConn_(client);
}


bool WebServer::initSocket_() {
    int ret;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = {0};
    if( openLinger_ ) {
        // 优雅关闭: 直到所剩数据发送完毕或超时
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET , SOCK_STREAM , 0);
    if( listenFd_ < 0 ) {
        return false;
    }

    //close后如socket存在数据则尽力发送
    //超时返回EWOULDBLOCK
    ret = setsockopt(listenFd_ , SOL_SOCKET , SO_LINGER , &optLinger , sizeof(optLinger));
    if( ret < 0 ) {
        close(listenFd_);
        return false;
    }

    int optval = 1;
    //端口复用
    ret = setsockopt(listenFd_ , SOL_SOCKET , SO_REUSEADDR , (const void*)&optval , sizeof(int));
    if( ret < 0 ){
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        close(listenFd_);
        return false;
    }
    ret = (int)epoller_->addFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        close(listenFd_);
        return false;
    }
    setFdNonblock(listenFd_);
    return true;
}

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


