#include "heaptimer.h"


HeapTimer::HeapTimer(){
	heap_.reserve(64);
}


void HeapTimer::add(int cfd , int timeOut , const TimeoutCallBack& cb){
	size_t index;
	//新节点
	if( ref_.count(cfd) == 0 ){
		index = heap_.size();
		ref_[cfd] = index;
		heap_.push_back({cfd , (Clock::now() + MS(timeOut)) , cb});
		siftUp_(index);
	} 
	//旧节点
	else{
		index = ref_[cfd];
		heap_[index].expires = Clock::now() + MS(timeOut);
		heap_[index].cb = cb;
		if( !siftDown_(index , heap_.size()) ){
			siftUp_(index);
		}
	}
}

void HeapTimer::siftUp_(size_t index){
	size_t parent = (index - 1) / 2;
	//(0-1)/2=0;
	//另size_t <0 时，会越界
	while( index!=0 && parent >= 0 ){
		if( heap_[parent] < heap_[index] ) break;
		swapNode_(parent , index);
		index = parent;
		parent = (index - 1) / 2;
	}
}

bool HeapTimer::siftDown_( size_t index,size_t num){
	size_t cur = index;
	size_t child = index * 2 + 1;
	while( child < num ){
		//右小于左 
		if( child + 1 < num && heap_[child + 1] < heap_[child] ) child++;
		//父小于子
		if( heap_[cur] < heap_[child] ) break;
		swapNode_(cur , child);
		cur = child;
		child = cur * 2 + 1;
	}
	return cur > index;
}

void HeapTimer::swapNode_(size_t index1 , size_t index2){
	std::swap(heap_[index1] , heap_[index2]);
	//重新建立cfd和index的关系
	ref_[heap_[index1].cfd] = index1;
	ref_[heap_[index2].cfd] = index2;
}


void HeapTimer::adjust(int cfd , int timeOut){
	heap_[ref_[cfd]].expires = Clock::now() + MS(timeOut);
	siftDown_(ref_[cfd] , heap_.size());
}


void HeapTimer::del_(size_t index){
	size_t tail = heap_.size() - 1;
	if( index < tail ){
		//待删除节点放在容器末尾，调整曾经的尾节点
		swapNode_(index , tail);
		if( !siftDown_(index , tail) ){
			siftUp_(index);
		}
	}
	ref_.erase(heap_[tail].cfd);
	heap_.pop_back();
}



int HeapTimer::getNextTick(){
	tick();
	int res = -1;
	if( !heap_.empty() ){
		res = (int)std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
		if( res < 0 ){
			res = 0;
		}
	}
	return res;
}


void HeapTimer::tick(){
	
	while( !heap_.empty() ){
		TimerNode node = heap_.front();
		if( (node.expires - Clock::now()).count() > 0 ) break;
		node.cb();
		del_(0);
	}
}



#include <iostream>
void HeapTimer::textTime(){
	std::cout <<
		std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count() 
		<< std::endl;
}
