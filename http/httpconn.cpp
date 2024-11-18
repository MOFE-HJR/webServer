#include "httpconn.h"
using namespace std;

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
};

HttpConn::~HttpConn() { 
    closeConn(); 
};

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.cleanBuffer();
    readBuff_.cleanBuffer();
    isClose_ = false;
}

void HttpConn::closeConn() {
    response_.unMapFilePtr();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
    }
}

int HttpConn::getFd() const {
    return fd_;
};


ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.readFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writeBuff_.writeFdByIov(fd_, iov_, iovCnt_,saveErrno);
        if( len <= 0 || getIovLen() == 0 ) {
            break;
        }
    } while(isET);
    return len;
}

bool HttpConn::process() {
    request_.init();
    if(readBuff_.readableBytes() <= 0) {
        return false;
    }
    else if(request_.parse(readBuff_)) {
        //200 OK
        response_.init(srcDir, request_.path(), request_.isKeepAlive(), 200);
    } else {
        //400 bad request 客户端语法错误
        response_.init(srcDir, request_.path(), false, 400);
    }

    response_.makeResponse(writeBuff_);
    //分两部分写相应报文 0写标准格式 1写文件内容
    iov_[0].iov_base = writeBuff_.beginReadPtr();
    iov_[0].iov_len = writeBuff_.readableBytes();
    iovCnt_ = 1;

    if(response_.getFileLen() > 0  && response_.getFilePtr()) {
        iov_[1].iov_base = response_.getFilePtr();
        iov_[1].iov_len = response_.getFileLen();
        iovCnt_ = 2;
    }
    return true;
}

int HttpConn::getIovLen(){
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConn::isKeepAlive() const{
    return request_.isKeepAlive();
}