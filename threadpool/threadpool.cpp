
#include <iostream>
#include <thread>
#include <chrono>

#include "threadpool.h"

Semaphore_::Semaphore_(int limit)
	:limit_(limit)
	,isRunning(true)
{}

Semaphore_::~Semaphore_(){
	isRunning = false;
}

void Semaphore_::wait(){
	if( !isRunning )
		return;
	std::unique_lock<std::mutex> lock(mtx_);
	cond_.wait(lock , [&]()->bool{ return limit_ > 0; });
	limit_--;
}

void Semaphore_::post(){
	if( !isRunning )
		return;
	std::unique_lock<std::mutex> lock(mtx_);
	limit_++;
	cond_.notify_all();
}



Result_::Result_(std::shared_ptr<Task_> task , bool isVaild)
	:task_(task)
	,isVaild_(isVaild)
{	
	task_->setResult(this);
}

Any_ Result_::get(){
	if( !isVaild_ ){
		return "";
	}
	sem_.wait();
	return std::move(any_);
}

void Result_::setVal(Any_&& any){
	any_ = std::move(any);
	sem_.post();
}




Task_::Task_()
	:result_(nullptr)
{}

void Task_::exec(){
	if( result_ != nullptr ){
		result_->setVal(std::move(run()));
	}
}

void Task_::setResult(Result_* result){
	result_ = result;
}




size_t Thread_::generateId_ = 0;
Thread_::Thread_(ThreadFuc func)
	:func_(func)
	,id_(generateId_++)
{}

Thread_::~Thread_()
{}

void Thread_::start(){
	std::thread t(func_,id_);
	t.detach();
}

size_t Thread_::getId() const{
	return id_;
}





const size_t TASK_QUE_MAX = INT32_MAX;		
const size_t THREAD_MAX = 1024;				
const size_t THREAD_MAX_IDLE_TIME = 60;		
ThreadPool_::ThreadPool_()
	:initThreadSize_(std::thread::hardware_concurrency())
	,idleThreadSize_(0)
	,threadMax_(THREAD_MAX)
	,threadSize_(0)
	,taskSize_(0)
	,taskQueMax_(TASK_QUE_MAX)
	,mode_(PoolMode_::MODE_FIXED)
	,isPoolRunning_(false)
{}

ThreadPool_::~ThreadPool_(){
	isPoolRunning_ = false;
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	//唤起所有因为taskQue为空而阻塞的线程，执行销毁
	notEmpty_.notify_all();
	//等待所有线程销毁
	exitCond_.wait(lock , [&]()->bool{ return threads_.size() == 0; });

}

void ThreadPool_::setInitThradSize(size_t initThreadSize){
	if( isPoolRunning_ )
		return;
	initThreadSize_ = initThreadSize;
}

void ThreadPool_::setTaskQueMax(size_t taskQueMax){
	if( isPoolRunning_ )
		return;
	taskQueMax_ = taskQueMax;
}

void ThreadPool_::setMode(PoolMode_ mode){
	if( isPoolRunning_ )
		return;
	mode_ = mode;
}

void ThreadPool_::setThreadMax(size_t threadMax){
	if( isPoolRunning_ )
		return;
	if(mode_ == PoolMode_::MODE_CACHED )
		threadMax_ = threadMax;
}

bool ThreadPool_::isRunning(){
	return isPoolRunning_;
}

Result_ ThreadPool_::submitTask(std::shared_ptr<Task_> task){

	std::unique_lock<std::mutex> lock(taskQueMtx_);

	if( !(notFull_.wait_for(lock , std::chrono::seconds(1)
			, [&]()->bool{ return taskQueMax_ > taskQue_.size(); }))){
		std::cout << "task queue is full,submit task fail" << std::endl;

		return Result_(task,false);
	}

	taskQue_.emplace(task);
	taskSize_++;
	notEmpty_.notify_all();

	//cached模式下可以增加线程
	if( mode_ == PoolMode_::MODE_CACHED 
		&& taskQue_.size() > idleThreadSize_
		&& threadSize_ < threadMax_){
		auto ptr = std::make_unique<Thread_>(std::bind(&ThreadPool_::threadFunc
			, this , std::placeholders::_1));
		if(ptr == nullptr) std::cout<<"ptr == null"<<std::endl;
		size_t tmpId = ptr->getId();
		std::cout << tmpId<<std::endl;
		threads_.emplace(tmpId , std::move(ptr));
		threads_[tmpId]->start();
		
		threadSize_++;
		idleThreadSize_++;
		std::cout << "create new thread" << std::endl;
	}

	return Result_(task);
}

void ThreadPool_::start(size_t iniThradSize){

	if( iniThradSize > 0 ){
		initThreadSize_ = iniThradSize;
	}
	threadSize_ = (unsigned int )initThreadSize_;
	isPoolRunning_ = true;
	
	for( size_t i = 0; i < initThreadSize_; ++i ){
		auto ptr = std::make_unique<Thread_>(std::bind(&ThreadPool_::threadFunc
			, this , std::placeholders::_1));
		size_t tmpId = ptr->getId();
		threads_.emplace(tmpId , std::move(ptr));
	}

	for( size_t i = 0; i < initThreadSize_; ++i ){
		threads_[i]->start();
		idleThreadSize_++;
	}
}

void ThreadPool_::threadFunc(size_t id){
	
	auto lastTime = std::chrono::high_resolution_clock().now();

	while( 1 ){

		std::shared_ptr<Task_> task;
		std::unique_lock<std::mutex> lock(taskQueMtx_);

		while( taskQue_.size() == 0 ){

			if( !isPoolRunning_ ){
				threads_.erase(id);
				std::cout << "delete thread:" << std::this_thread::get_id() << std::endl;
				exitCond_.notify_all();
				return;
			}

			//销毁多余线程
			if( mode_ == PoolMode_::MODE_CACHED ){

				//超时检测每秒一次
				if( std::cv_status::timeout ==
					notEmpty_.wait_for(lock , std::chrono::seconds(1)) ){
					auto nowTime = std::chrono::high_resolution_clock::now();
					auto durTime = std::chrono::duration_cast<std::chrono::seconds>
						(nowTime - lastTime);

					//超时（默认60s）
					if( durTime.count() >= (int64_t)THREAD_MAX_IDLE_TIME
						&& threadSize_ > initThreadSize_ ){
						threads_.erase(id);
						threadSize_--;
						idleThreadSize_--;
						std::cout << "delete thread:" << std::this_thread::get_id() << std::endl;
						return;
					}
				}
			}

			else{
				notEmpty_.wait(lock);
			}

		}

		//线程获取任务
		task = taskQue_.front();
		taskQue_.pop();
		taskSize_--;
		idleThreadSize_--;
		
		lock.unlock();
		if( taskQue_.size() > 0 ){
			notEmpty_.notify_all();
		}
		notFull_.notify_all();
		
		if( task != nullptr ){
			task->exec();
		}
		idleThreadSize_++;
		//更新时间
		lastTime = std::chrono::high_resolution_clock().now();
	}
}

