#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>

//线程池工作模式
enum class PoolMode_{
	MODE_FIXED,
	MODE_CACHED
};


//仿写Any
class Any_{
public:
	Any_() = default;
	~Any_() = default;
	Any_(const Any_&) = delete;
	Any_& operator=(const Any_&) = delete;
	Any_(Any_&&) = default;
	Any_& operator=(Any_&&) = default;

	
	template<typename T>
	Any_(T data):base_(std::make_unique<Derive_<T>>(data))
	{}

public:
	//提供接口获取数据
	template<typename T>
	T cast_(){
		Derive_<T>* son = dynamic_cast<Derive_<T>*>(base_.get());
		if( son == nullptr ){
			throw "type is not match!!!";
		}
		return son->getData();
	}

private:
	class Base_{
	public:
		virtual ~Base_() = default;
	};

	template<typename T>
	class Derive_ :public Base_{
	public:
		Derive_(T data)
			:data_(data)
		{}
	public:
		T getData(){
			return data_;
		}
	private:
		T data_;
	};

private:
	std::unique_ptr<Base_> base_;
};


//仿写信号量类
class Semaphore_{
public:
	Semaphore_(int limit = 0);
	~Semaphore_();

public:
	void wait();
	void post();

private:
	size_t limit_;
	std::mutex mtx_;
	std::condition_variable cond_;

	//Linux下条件变量析构不释放资源会造成死锁
	//增加判断条件可以避免死锁
	//Window下可以不用
	bool isRunning;
};



class Task_;
class Result_{
public:
	Result_() = default;
	~Result_() = default;

	Result_(std::shared_ptr<Task_> task , bool isVaild = true);

public:
	//获取任务执行完结果
	Any_ get();

	//设置任务执行完的结果
	void setVal(Any_&& any);

private:
	Any_ any_;					
	Semaphore_ sem_;				
	std::shared_ptr<Task_> task_;//存储提交的任务
	std::atomic_bool isVaild_;//判断提交任务是否有效
};


class Task_{
public:
	Task_();
	~Task_() = default;

public:
	//执行任务
	void exec();

	//关联task和result
	void setResult(Result_* result);

	//用户自定义线程的执行任务
	virtual Any_ run() = 0;

private:
	Result_* result_;
};


class Thread_{
public:
	using ThreadFuc = std::function<void(size_t)>;
	Thread_(ThreadFuc func);
	~Thread_();

public:
	//开启线程
	void start();
	
	//获取线程编号
	size_t getId() const;

private:
	ThreadFuc func_;//线程执行函数		
	static size_t generateId_;//给线程编号赋值
	size_t id_;//线程编号		
};



class ThreadPool_{
public:
	ThreadPool_();
	~ThreadPool_();
	ThreadPool_ (const ThreadPool_&) = delete;
	ThreadPool_& operator=(const ThreadPool_&) = delete;

	void setInitThradSize(size_t initThreadSize);
	void setTaskQueMax(size_t taskQueMax);
	void setMode(PoolMode_ mode);
	void setThreadMax(size_t threadMax);
	
	bool isRunning();
	
	//提交任务
	Result_ submitTask(std::shared_ptr<Task_> task);

	//线程池开启
	void start(size_t iniThradSize = 0);

private:
	//线程执行函数
	void threadFunc(size_t id);

private:
	
	std::unordered_map<size_t , std::unique_ptr<Thread_>> threads_;	
	size_t initThreadSize_;											
	std::atomic_uint idleThreadSize_;								
	size_t threadMax_;												
	std::atomic_uint threadSize_;									

	std::queue<std::shared_ptr<Task_>> taskQue_;					
	std::atomic_uint taskSize_;										
	size_t taskQueMax_;												

	std::mutex taskQueMtx_;											
	std::condition_variable notFull_;								
	std::condition_variable notEmpty_;								
	std::condition_variable exitCond_;//执行完线程任务才可以销毁线程池						

	PoolMode_ mode_;												
	std::atomic_bool isPoolRunning_;								

};

#endif

