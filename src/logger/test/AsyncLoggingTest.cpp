/*
muduo 日志库由前端和后端组成。
1. 前端主要包括：Logger、LogStream、FixedBuffer、SourceFile。
2. 后端主要包括：AsyncLogging、LogFile、AppendFile。
前端主要实现异步日志中的日志功能，为用户提供将日志内容转换为字符串，封装为一条完整的 log 消息存放到 FixedBuffer 中；
而实现异步，核心是通过专门的后端线程，与前端线程并发运行，将 FixedBuffer 中的大量日志消息写到磁盘上。
1. AsyncLogging 提供后端线程，定时将 log 缓冲写到磁盘，维护缓冲及缓冲队列。
2. LogFile 提供日志文件滚动功能，写文件功能。
3. AppendFile 封装了OS 提供的基础的写文件功能。
*/

#include <stdio.h>
#include <unistd.h>

#include "AsyncLogging.h"
#include "Logging.h"
#include "Timestamp.h"

static const off_t kRollSize = 1 * 1024 * 1024;
AsyncLogging* g_asyncLog = NULL;

inline AsyncLogging* getAsyncLog()
{
    return g_asyncLog;
}

void test_Logging()
{
    LOG_DEBUG << "debug";
    LOG_INFO << "info";
    LOG_WARN << "warn";
    LOG_ERROR << "error";
    // 注意不能轻易使用 LOG_FATAL, LOG_SYSFATAL, 会导致程序abort

    const int n = 10;
    for (int i = 0; i < n; ++i) {
        LOG_INFO << "Hello, " << i << " abc...xyz";
    }
}

void test_AsyncLogging()
{
    const int n = 1024;
    for (int i = 0; i < n; ++i) {
        LOG_INFO << "Hello, " << i << " abc...xyz";
    }
}

void asyncLog(const char* msg, int len)
{
    AsyncLogging* logging = getAsyncLog();
    if (logging) {
        logging->append(msg, len);
    }
}

int main(int argc, char* argv[])
{
    printf("pid = %d\n", getpid());

    AsyncLogging log(::basename(argv[0]), kRollSize);
    test_Logging();

    sleep(1);

    g_asyncLog = &log;
    Logger::setOutput(asyncLog); // 为Logger设置输出回调, 重新配接输出位置
    log.start(); // 开启日志后端线程

    test_Logging();
    test_AsyncLogging();

    sleep(1);
    log.stop();
    return 0;
}