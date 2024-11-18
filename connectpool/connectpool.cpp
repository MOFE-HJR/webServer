#include <thread>
#include <stdio.h>
#include "connectpool.h"
#include "tool.h"

using namespace std;

ConnectPool_::ConnectPool_(){

	if( !loadConnectPool() ){
		return;
	}

	for( int i = 0; i < initPoolSize_; ++i ){
		Connection_* p = new Connection_();
		if(p->connect(ip_, port_
		 , username_ , password_
		  , dbname_)){
		}
		p->refreshTime();
		connQue_.push(p);

		connQueSize_++;
	}

	//另开两个线程去执行创建连接、检测超时的任务
	thread produce(bind(&ConnectPool_::createNewConn , this));
	produce.detach();

	thread scaner(bind(&ConnectPool_::scanTimeOutConn , this));
	scaner.detach();
}


ConnectPool_* ConnectPool_::getConnectPool(){
	static ConnectPool_ pool;
	return &pool;
}


shared_ptr<Connection_> ConnectPool_::getConnect(){
	unique_lock<mutex> lock(connQueMtx_);
	while( connQue_.empty() ){
		if( cv_status::timeout == empty_.wait_for(lock , std::chrono::milliseconds(100)) ){
			if( connQue_.empty() ){
				return nullptr;
			}
		}
	}

	//自定义删除器，把不用的连接归还池中
	shared_ptr<Connection_> sp(connQue_.front() , [&](Connection_* p){ 
		unique_lock<mutex> lock(connQueMtx_); 
		p->refreshTime();
		connQue_.push(p);
		connQueSize_++;
		});
	connQue_.pop();
	connQueSize_--;
	empty_.notify_all();

	return sp;
}

bool ConnectPool_::loadConnectPool_(){
	char tmp[] ="mysql.ini";
	FILE* file = fopen(tmp , "r");
	if( file == nullptr ){
		perror("open err");
		return false;
	}

	char buf[1024] = {0};
	while( fgets(buf , 1024 , file) != nullptr ){
		string line = buf;
		size_t front = line.find('=' , 0);
		if( (int)front == -1 ){
			continue;
		}
		//Linux下使用 ‘\r’
		//Window下使用‘\n’
		size_t end = line.find('\r' , front);
		string key = line.substr(0 , front);
		string value = line.substr(front + 1 , end - front - 1);
		//cout << key << ":" << value << endl;

		if( key == "ip" ){
			ip_ = value;
		}
		else if( key == "port" ){
			port_ = atoi(value.c_str()); 
		}
		else if( key == "username" ){
			username_ = value; 
		}
		else if( key == "password" ){
			password_ = value;
		}
		else if( key == "dbname" ){
			dbname_ = value;
		}
		else if( key == "initPoolSize" ){
			initPoolSize_ = atoi(value.c_str());
		}
		else if( key == "maxPoolSize" ){
			maxPoolSize_ = atoi(value.c_str());
		}
		else if( key == "maxIdleTime" ){
			maxIdleTime_ = atoi(value.c_str());
		}
		else if( key == "connectTimeOut" ){
			connectTimeOut_ = atoi(value.c_str());
		}
	}

	return true;
}


void ConnectPool_::createNewConn_(){
	while( 1 ){
		unique_lock<mutex> lock(connQueMtx_);
		while( !connQue_.empty() ){
			empty_.wait(lock);
		}
		if( connQueSize_ < maxPoolSize_ ){
			Connection_* p = new Connection_();
			p->connect(ip_, port_ , username_, 
			password_, dbname_);
			p->refreshTime();
			connQue_.push(p);
			connQueSize_++;
		}
	}
}


void ConnectPool_::scanTimeOutConn_(){
	while( 1 ){
		this_thread::sleep_for(chrono::seconds(maxIdleTime_));

		unique_lock<mutex> lock(connQueMtx_);
		while( connQueSize_ > initPoolSize_ ){
			Connection_* p = connQue_.front();
			if( p->getAliveTime() >= maxIdleTime_ * 1000 ){
				connQue_.pop();
				delete p;
				connQueSize_--;
			}
		}
	}
}
