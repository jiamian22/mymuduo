#include "EPollPoller.h"
#include <string.h>

const int kNew = -1;    // 某个channel还没添加至Poller（channel的成员index_初始化为-1）
const int kAdded = 1;   // 某个channel已经添加至Poller
const int kDeleted = 2; // 某个channel已经从Poller删除

// TODO:epoll_create1(EPOLL_CLOEXEC)
EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop)  // 传给基类
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0) {
        LOG_FATAL << "epoll_create() error:" << errno;
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// 该方法内部调用 epoll_wait 获取发生的事件，并找到这些事件对应的 Channel 并将这些活跃的 Channel 填充入 activeChannels 中，最后返回一个时间戳。
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 高并发情况经常被调用，影响效率，使用debug模式可以手动关闭

    size_t numEvents = ::epoll_wait(epollfd_, &(*events_.begin()), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;  // 记录原始的erron（记日志用），防止运行过程中其他线程改变该erron的值
    Timestamp now(Timestamp::now());

    // 有事件产生
    if (numEvents > 0) {
        fillActiveChannels(numEvents, activeChannels);  // 填充活跃的channels
        // 对events_进行扩容操作
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {  // 超时
        LOG_DEBUG << "timeout!";
    } else {  // 出错
        // 不是终端错误
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR << "EPollPoller::poll() failed";
        }
    }
    return now;
}

/**
 * Channel::update => EventLoop::updateChannel => Poller::updateChannel
 * Channel::remove => EventLoop::removeChannel => Poller::removeChannel
 */
// 获取 channel 在 EPollPoller 上的状态，根据状态进行不同操作。最后调用 update 私有方法。
void EPollPoller::updateChannel(Channel* channel)
{
    // TODO:__FUNCTION__
    // 获取参数channel在epoll的状态
    const int index = channel->index();
    
    // 未添加状态和已删除状态都有可能会被再次添加到epoll中
    if (index == kNew || index == kDeleted) {
        // 添加到键值对 
        if (index == kNew) {
            int fd = channel->fd();
            // assert(channels_.find(fd) == channels_.end());  // 断言在当前的map中找不到此fd
            channels_[fd] = channel; 
        } else {  // index == kDeleted
            // 断言或者报错
            // int fd = channel->fd();
            // assert(channels_.find(fd) != channels_.end());  // 被删除则从epoll对象里被删除了，但是map中仍然留存
            // assert(channels_[fd] == channel);  // 继续断言，该键值对应该是对应的
        }
        // 修改channel的状态，此时是已添加状态
        channel->set_index(kAdded);
        // 向epoll对象加入channel
        update(EPOLL_CTL_ADD, channel);
    } else {  // index == kAdded，channel已经在poller上注册过
        // int fd = channel->fd();
        // assert(channels_.find(fd) != channels_.end());  // 断言可以在map找到
        // assert(channels_[fd] == channel);  // 断言键值对相等
        // assert( index == kAdded);  // 断言channel在poller的装填

        // 没有感兴趣事件说明可以从epoll对象中删除该channel了
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {  // 如果还有处理事件，则更改监视事件的类型
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 填写活跃的连接
// 通过 epollwait 返回的 events 数组内部有指向 channel 的指针，我们可以通过此指针在 EPollPoller 模块获取对 channel 进行操作。
// 我们需要更新 channel 的返回事件的设置，并且将此 channel 装入 activeChannels。
void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for (int i = 0; i < numEvents; ++i) {
        // void* => Channel*
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);  // 更改此 channel 的事件
        activeChannels->push_back(channel);  // 加入activeChannels
    }
}

// 从 epoll 中移除监视的channel
void EPollPoller::removeChannel(Channel* channel)
{
    // 从map中删除
    int fd = channel->fd();
    channels_.erase(fd); 

    int index = channel->index();  // 获取此channel在EPollPoller对象中的状态
    if (index == kAdded) {
        // 如果此fd已经被添加到Poller中，则还需从epoll对象中删除
        update(EPOLL_CTL_DEL, channel);
    }
    // 重新设置channel的状态为未被Poller注册
    channel->set_index(kNew);
}

// 本质是调用 epoll_ctl 函数，而里面的操作由之前的 updateChannel 所指定
void EPollPoller::update(int operation, Channel* channel)
{
    // 获取一个epoll_event
    epoll_event event;
    ::memset(&event, 0, sizeof(event));

    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;  // event的成员保存channel指针（void*）

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR << "epoll_ctl() del error:" << errno;
        } else {
            LOG_FATAL << "epoll_ctl add/mod error:" << errno;
        }
    }
}