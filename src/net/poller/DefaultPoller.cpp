#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

// 获取默认的Poller实现方式
Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL")) {  // shell的环境变量（一般不会设置）
        // return new PollPoller(loop);  // 生成poll实例
        return nullptr;
    } else {
        return new EPollPoller(loop); // 生成epoll实例
    }
}

