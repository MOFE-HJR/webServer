#ifndef _CONNECTPOOL_H_
#define _CONNECTPOOL_H_

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>

#include "connection.h"

using namespace std;

class ConnectPool_{
public:
	//单例模式
	static ConnectPool_* getConnectPool();

	//从池中获取一个连接
	shared_ptr<Connection_> getConnect();

	//返回连接池中连接的数量
	int getConnQueSize(){
		return connQueSize_;
	}
	
private:
	ConnectPool_();
	~ConnectPool_() = default;

	//根据mysql.ini文件初始化连接所需要的信息
	bool loadConnectPool_();

	//向连接池添加新连接
	void createNewConn_();

	//检测持有的连接是否超时
	void scanTimeOutConn_();

	
private:
	string ip_;
	string username_;
	string password_;
	string dbname_;
	unsigned short port_;
	int initPoolSize_;
	int maxPoolSize_;
	int maxIdleTime_;
	int connectTimeOut_;

	queue<Connection_*> connQue_;
	mutex connQueMtx_;
	condition_variable empty_;
	atomic_int connQueSize_;
};

#endif

