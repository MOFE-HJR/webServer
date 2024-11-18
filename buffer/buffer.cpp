#include "buffer.h"

Buffer::Buffer(int initBuffSize){
	buffer_.resize(initBuffSize);
	writePos_ = 0;
	readPos_ = 0;
}


ssize_t Buffer::readFd(int fd , int* saveErrno)
{
	char tmp[65535];
	struct iovec iov[2];
    const size_t writable = writableBytes();
    iov[0].iov_base = beginWritePtr();
    iov[0].iov_len = writable;
    iov[1].iov_base = tmp;
    iov[1].iov_len = sizeof(tmp);

    const ssize_t len = readv(fd , iov , 2);
    if( len < 0 ) {
        *saveErrno = errno;
    } else if( static_cast<size_t>(len) <= writable ) {
        offWritePos(len);
    } else {
        writePos_ = buffer_.size();
        appendSpace_(tmp , len - writable);
    }
    return len;
}


ssize_t Buffer::writeFd(int fd , int* saveErrno)
{
    size_t readSize = readableBytes();
    ssize_t len = write(fd , beginReadPtr() , readSize);
    if( len < 0 ) {
        *saveErrno = errno;
        return len;
    }
    offReadPos(len);
    return len;
}


ssize_t Buffer::writeFdByIov(int fd,struct iovec* iov,int iovCnt,int* err){
	ssize_t len=-1;
    len=writev(fd,iov,iovCnt);
    if(len<=0) return len;

	if(static_cast<size_t>(len) > iov[0].iov_len) {
    		//如果写完报头
    		iov[1].iov_base = (u_int8_t*)iov[1].iov_base + (len - iov[0].iov_len);
    		iov[1].iov_len -= (len - iov[0].iov_len);
   		 if(iov[0].iov_len) {
     			   //置空
       			 cleanBuffer();
       			 iov[0].iov_len = 0;
    		}
	}
	else {
    		//报头没有写完
    		iov[0].iov_base = (u_int8_t*)iov[0].iov_base + len; 
    		iov[0].iov_len -= len; 
   		    offReadPos(len);
	}
	return len;
}


void Buffer::appendSpace_(const char* str , size_t len){
    makeSpace_(len);
    std::copy(str , str + len , beginWritePtr());
    offWritePos(len);
}


void Buffer::makeSpace_(size_t len){
    //释放已读部分空间不够用于写新数据
    //增加缓冲区大小
    if( readPos_ < len ){
        buffer_.resize(writePos_ + len + 1);
    } 
    //够，调整缓冲区
    else{
        size_t rdable = readableBytes();
        std::copy(beginReadPtr() , beginWritePtr() , beginPtr_());
        readPos_ = 0;
        writePos_ = rdable;
    }
}


size_t Buffer::writableBytes() const{
    return buffer_.size() - writePos_;
}


size_t Buffer::readableBytes() const{
    return writePos_ - readPos_;
}


char* Buffer::beginPtr_(){
    return &(*(buffer_.begin()));
}


char* Buffer::beginWritePtr() 
{
    return (beginPtr_() + writePos_);
}


char* Buffer::beginReadPtr() 
{
    return (beginPtr_() + readPos_);
}

void Buffer::offWritePos(size_t len){
    writePos_ += len;
}

void Buffer::offReadPos(size_t len){
    readPos_ += len;
}

void Buffer::offReadPosByPtr(char* end){
    if(beginReadPtr() < end){
        offReadPos(end - beginReadPtr());
    }
}

void Buffer::cleanBuffer(){
    bzero(&(buffer_[0]) , buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::cleanBufGetStr(){
    return std::string();
}

void Buffer::appendByStr(const std::string& str){
    appendSpace_(str.data(),str.length());
}

void Buffer::testBuf(){
    for(auto ch:buffer_){
        std::cout<<ch<< "";
    }
    std::cout<< std::endl;
}




















