#ifndef _HEAPTIMER_H_
#define _HEAPTIMER_H_


#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <functional> 
#include <assert.h> 
#include <chrono>
#include <vector>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;//00:00

struct TimerNode {
    int cfd;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};
class HeapTimer {
public:
    HeapTimer();

    ~HeapTimer() = default;

    //延长客户端连接时间
    void adjust(int cfd , int timeOut);

    //添加客户端时间节点
    void add(int cfd , int timeOut , const TimeoutCallBack& cb);

    //处理超时节点
    void tick();

    //返回最近一个未超时节点的剩余时长
    int getNextTick();

    //测试
    void textTime();

private:
    //清除节点
    void del_(size_t index);

    //上升
    void siftUp_(size_t index);

    //下降
    bool siftDown_(size_t index , size_t num);

    //交换
    void swapNode_(size_t index1 , size_t index2);

    //最小堆
    std::vector<TimerNode> heap_;

    //存储cfd和index的关系，index对应堆的index
    std::unordered_map<int , size_t> ref_;
};


#endif // !_HEAPTIMER_H_

