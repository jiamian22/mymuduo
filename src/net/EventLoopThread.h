#ifndef __EVENT_LOOP_THREAD_H__
#define __EVENT_LOOP_THREAD_H__

#include <functional>
#include <mutex>
#include <condition_variable>

#include "noncopyable.h"
#include "Thread.h"

// one loop per thread
class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    // cb 的默认值为ThreadInitCallback类型的默认构造函数的结果，创建一个未绑定的函数对象，也就是说它不指向任何函数。
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());

    ~EventLoopThread();

    // 开启线程
    EventLoop* startLoop();

private:
    void threadFunc();

    // EventLoopThread::threadFunc 会创建 EventLoop
    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};



#endif  // __EVENT_LOOP_THREAD_H__