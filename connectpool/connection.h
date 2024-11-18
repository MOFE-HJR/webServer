#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <string>
#include <ctime>
#include <mysql/mysql.h>

using namespace std;

class Connection_{
public:
	Connection_();
	~Connection_();

public:
	//建立连接
	bool connect(string ip
	,unsigned short port 
	,string user 
	,string password 
	,string dbname);

	//执行语句
	bool update(string sql);

	//获得结果集
	MYSQL_RES* query(string sql);

	//获得结果集列数
	unsigned int getFieldNum(MYSQL_RES* res);

	//获得结果集行数
	uint64_t getRowNum(MYSQL_RES* res);

	//获得结果集的一行
	MYSQL_ROW getResRow(MYSQL_RES* res);

	//清空结果集
	void freeRes(MYSQL_RES* res);


	//更新时间
	void refreshTime(); 
		
	//获得剩余存活时间
	clock_t getAliveTime() const;

private:
	MYSQL* conn_;//SQL句柄
	clock_t aliveTime_;//记录获取连接的时间
};


#endif

