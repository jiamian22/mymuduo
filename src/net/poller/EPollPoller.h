#ifndef __EPOLLPOLLER_H__
#define __EPOLLPOLLER_H__

#include <vector>
#include <sys/epoll.h>
#include <unistd.h>

#include "Logging.h"
#include "Poller.h"
#include "Timestamp.h"

/**
 * epoll_create
 * epoll_ctl  add/mod/del
 * epoll_wait
 */
class EPollPoller : public Poller
{
    using EventList = std::vector<epoll_event>;  // epoll_event数组
public:
    EPollPoller(EventLoop* Loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    // 默认监听事件数量
    static const int kInitEventListSize = 16; 

    // 填充活跃的连接
    // EventLoop内部调用此方法，会将发生了事件的Channel填充到activeChannels中
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    // 更新channel通道，本质是调用了epoll_ctl
    void update(int operation, Channel* channel);

    int epollfd_;       // epoll_create在内核创建空间返回的fd
    EventList events_;  // 用于存放epoll_wait返回的所有发生的事件的文件描述符
};

#endif // __EPOLLPOLLER_H__