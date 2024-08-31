#ifndef __ASYNC_LOGGING_H__
#define __ASYNC_LOGGING_H__

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "noncopyable.h"
#include "Thread.h"
#include "FixedBuffer.h"
#include "LogStream.h"
#include "LogFile.h"

class AsyncLogging
{
public:
    AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_) {
            stop();
        }
    }

    // 前端调用 append 写入日志
    void append(const char* logling, int len);

    void start()
    {
        running_ = true;
        thread_.start();
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    // FixedBuffer缓冲区类型，而这个缓冲区大小由kLargeBuffer指定，大小为4M，因此，Buffer就是大小为4M的缓冲区类型
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;  // 等同于 std::unique_ptr<Buffer>

    void threadFunc();

    const int flushInterval_;  // 前端缓冲区定期向后端写入的时间（冲刷间隔）
    std::atomic<bool> running_;  // 标识线程函数是否正在运行
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;  // 条件变量，主要用于前端缓冲区队列中没有数据时的休眠和唤醒

    BufferPtr currentBuffer_;  // 当前使用的缓冲区，用于储存前端日志信息，如果不够则会使用预备缓冲区
    // 预备缓冲区，主要是在调用append向当前缓冲添加日志消息时，如果当前缓冲放不下，当前缓冲就会被移动到前端缓冲队列终端，此时预备缓冲区用来作为新的当前缓冲
    BufferPtr nextBuffer_;  // 预备缓冲区，用于储存前端日志信息，在第一个缓冲区不够时使用
    BufferVector buffers_;  // 前端缓冲区队列，用于储存前端日志信息，过一段时间或者缓冲区已满，就会将 Buffer 加入到 BufferVector 中。后端线程负责将 BufferVector 里的内容写入磁盘
};

#endif // __ASYNC_LOGGING_H__