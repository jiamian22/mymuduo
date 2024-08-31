#include "EventLoop.h"

#include "Logging.h"
#include "Poller.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include <fcntl.h>

// 防止一个线程创建多个EventLoop (thread_local)
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

//TODO:eventfd使用
int createEventfd()
{
    int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);  // 创建时指定其为非阻塞
    if (evfd < 0) {
        LOG_FATAL << "eventfd error: " << errno;
    }
    return evfd;
}

EventLoop::EventLoop()
    : looping_(false)  // 还未开启任务循环，设置为false
    , quit_(false)  // 还未停止事件循环
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())  // 获取当前线程tid
    , poller_(Poller::newDefaultPoller(this))  // 获取默认的poller
    , timerQueue_(new TimerQueue(this))  // 定时器管理对象
    , wakeupFd_(createEventfd())  // 创建eventfd作为线程间等待/通知机制
    , wakeupChannel_(new Channel(this, wakeupFd_))  // 封装wakeupFd成Channel
    , currentActiveChannel_(nullptr)  // 当前正执行的活跃Channel
{
    LOG_DEBUG << "EventLoop created " << this << " the index is " << threadId_;
    LOG_DEBUG << "EventLoop created wakeupFd " << wakeupChannel_->fd();
    // 已经保存了thread值，之前已经创建过了EventLoop，一个线程只能对应一个EventLoop，直接退出
    if (t_loopInThisThread) {
        LOG_FATAL << "Another EventLoop" << t_loopInThisThread << " exists in this thread " << threadId_;
    } else {
        // 线程静态变量保存tid值
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件的回调函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupChannel的EPOLLIN读事件
    // 如果有别的线程唤醒当前EventLoop，就会向wakeupFd_写数据，触发读操作
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    // channel移除所有感兴趣事件
    wakeupChannel_->disableAll();
    // 将channel从EventLoop中删除
    wakeupChannel_->remove();
    // 关闭 wakeupFd_
    ::close(wakeupFd_);
    // 指向EventLoop指针为空
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO << "EventLoop " << this << " start looping";

    while (!quit_) {
        // 清空activeChannels_
        activeChannels_.clear();
        // Poller监听哪些channel发生了事件 然后上报给EventLoop 通知channel处理相应的事件
        // 监听两类fd，一种是client的fd，一种是wakeup fd（mainLoop和subLoop之前通信的fd）
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel* channel : activeChannels_) {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * mainLoop：acceptSocket_接收连接 => 将acceptSocket_返回的connfd打包为Channel => TcpServer::newConnection通过轮询将TcpConnection对象分配给subLoop处理
         * mainLoop调用queueInLoop将回调加入subLoop（该回调需要subLoop执行 但subLoop还在poller_->poll处阻塞） queueInLoop通过wakeup将subLoop唤醒
         * 这些回调函数在 std::vector<Functor> pendingFunctors_; 之中
         */
        doPendingFunctors();
    }
    looping_ = false;
}

// 退出事件循环
// 1. loop在自己的线程中调用quit
// 2. 在非loop的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_ = true;

    /**
     * TODO:生产者消费者队列派发方式和muduo的派发方式
     * 有可能是别的线程调用quit(调用线程不是生成EventLoop对象的那个线程)
     * 比如在工作线程(subLoop)中调用了IO线程(mainLoop)的quit
     * 这种情况会唤醒主线程（从loop循环阻塞中跳出while判断quit_）
     */
    if (isInLoopThread()) {
        wakeup();
    }
}

// 在当前eventLoop中执行回调函数
// 在I/O线程中调用某个函数，该函数可以跨线程调用
void EventLoop::runInLoop(Functor cb)
{
    // 每个EventLoop都保存创建自己的线程tid
    // 我们可以通过CurrentThread::tid()获取当前执行线程的tid然后和EventLoop保存的进行比较
    if (isInLoopThread()) {
        // 如果当前调用runInLoop的线程（I/O线程）正好是EventLoop的运行线程，则直接同步调用此cb回调函数
        cb();
    } else {  // 在非当前eventLoop线程中执行回调函数，需要唤醒evevntLoop所在线程执行cb
        // 否则在其他线程中调用，就异步将cb添加到任务队列当中，
        // 以便让EventLoop真实对应的I/O线程执行这个回调函数
        queueInLoop(cb);
    }
}

// 将任务添加到队列当中，队就是成员pendingFunctors_数组容器
void EventLoop::queueInLoop(Functor cb)
{
    {
        // 操作任务队列需要保证互斥
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);  // 使用了std::move
    }

    // 唤醒相应的，需要执行上面回调操作的loop线程
    /**
     * std::atomic_bool callingPendingFunctors_; 标志当前loop正在执行functors中的回调中
     * 这个 || callingPendingFunctors_ 比较有必要，因为在执行回调的过程可能会向pendingFunctors_中加入新的回调
     * 则这个时候也需要唤醒，否则就会发生有事件到来但是仍被阻塞住的情况（如果不唤醒会造成本次发生的事件要伴随下次发生事件一起执行）
     * 如果是当前的IO线程调用且并没有正处理PendgingFunctors则不必唤醒
     */
    if (!isInLoopThread() || callingPendingFunctors_) {
        // 唤醒loop所在的线程
        wakeup();
    }
}

// 用来唤醒loop所在的线程的，向wakeupFd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    // 可以看到写的数据很少，纯粹是为了通知有事件产生
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR << "EventLoop::wakeup writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);    
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    /**
     * 如果没有生成这个局部的 functors，则在互斥锁加持下，我们直接遍历pendingFunctors，其他线程这个时候无法访问，无法向里面注册回调函数，增加服务器时延
     * 此外，交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
     */
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors) {
        functor();  // 执行当前loop需要执行的回调操作
    }
    // 记录此时结束了处理回调函数
    callingPendingFunctors_ = false;
}