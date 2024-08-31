#ifndef __POLLER_H__
#define __POLLER_H__

#include "noncopyable.h"
#include "Channel.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>


// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    // Poller关注的Channel
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* Loop);
    virtual ~Poller() = default;

    // 需要交给派生类实现的接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;  // 用于监听感兴趣的事件和fd(封装成了channel)
    virtual void updateChannel(Channel *channel) = 0;  // 更新事件，channel::update->eventloop::updateChannel->Poller::updateChannel（须在EventLoop所在的线程调用）
    virtual void removeChannel(Channel *channel) = 0;  // 当Channel销毁时移除此Channel（须在EventLoop所在的线程调用）

    // 判断一个 Channel 是否已经注册到当前 Poller 中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用实现方式(默认epoll)
    /** 
     * 它的实现并不在 Poller.cpp 文件中
     * 如果要实现则可以预料会其会包含EPollPoller PollPoller
     * 那么外面就会在基类引用派生类的头文件，这个抽象的设计就不好
     * 所以外面会单独创建一个 DefaultPoller.cpp 的文件去实现
     */
    static Poller* newDefaultPoller(EventLoop *Loop);

protected:  
    using ChannelMap = std::unordered_map<int, Channel*>;
    // 储存 channel 的映射，（sockfd -> channel*）
    ChannelMap channels_;
    
private:
    EventLoop* ownerLoop_; // 定义Poller所属的事件循环EventLoop
};

#endif // __POLLER_H__