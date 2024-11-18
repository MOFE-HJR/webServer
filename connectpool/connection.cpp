#include "connection.h"
#include "tool.h"
#include <iostream>

using namespace std;

Connection_::Connection_(){
	conn_ = mysql_init(nullptr);
	if(!conn_) std::cout<<"no init"<<std::endl;
	aliveTime_ = 0;
}

Connection_::~Connection_(){
	if( conn_ != nullptr ){
		mysql_close(conn_);
	}
}

bool Connection_::connect(string ip 
	,unsigned short port 
	,string user 
	,string password 
	,string dbname){

	MYSQL* sql = mysql_real_connect(conn_,ip.c_str(),user.c_str(),
	password.c_str(),dbname.c_str(),port,nullptr,0);

	if(!sql) LOG("real_connect err");
	return sql != nullptr;
}

bool Connection_::update(string sql){
	if( mysql_query(conn_ , sql.c_str() )){
		LOG("update err:" + sql);
		//std::cout<<"update err"<<std::endl;
		return false;
	}
	return true;
}

MYSQL_RES* Connection_::query(string sql){
	return mysql_store_result(conn_);
}

unsigned int Connection_::getFieldNum(MYSQL_RES* res){
	if( res ){
		return mysql_num_fields(res);
	}
	return 0;
}

uint64_t Connection_::getRowNum(MYSQL_RES* res){
	if( res ){
		return mysql_num_rows(res);
	}
	return 0;
}

MYSQL_ROW Connection_::getResRow(MYSQL_RES* res){
	if( res ){
		return mysql_fetch_row(res);
	}
	return nullptr;
}

void Connection_::freeRes(MYSQL_RES* res){
	if( res ){
		mysql_free_result(res);
	}
}

void Connection_::refreshTime(){
	aliveTime_ = clock();
}

clock_t Connection_::getAliveTime() const{
	return clock() - aliveTime_;
}
