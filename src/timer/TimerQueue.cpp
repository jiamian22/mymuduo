#include "EventLoop.h"
#include "Channel.h"
#include "Logging.h"
#include "Timer.h"
#include "TimerQueue.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

int createTimerfd()
{
    /**
     * 当超时事件发生时，该文件描述符就变为可读。
     * 这种特性可以使我们在写服务器程序时，很方便的便把定时事件变成和其他I/O事件一样的处理方式，
     * 当时间到期后，就会触发读事件。我们调用响应的回调函数即可。
     * CLOCK_REALTIME：相对时间   CLOCK_MONOTONIC：绝对时间    TFD_NONBLOCK：非阻塞
     */
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);  // 成功返回0
    if (timerfd < 0) {
        LOG_ERROR << "Failed in timerfd_create";
    }
    return timerfd;
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop)
    , timerfd_(createTimerfd())
    , timerfdChannel_(loop_, timerfd_)
    , timers_()
{
    // 设置timerfdChannel绑定读事件，并置于可读状态
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{   
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    // 删除所有定时器
    for (const Entry& timer : timers_) {
        delete timer.second;
    }
}

// 插入定时器
// 加入一个定时器事件，会向里传入定时器回调函数，超时时间和间隔时间（为0.0则为一次性定时器），addTimer方法根据这些属性构造新的定时器
void TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    // 是否取代了最早的定时触发时间
    bool eraliestChanged = insert(timer);

    // 我们需要重新设置timerfd_触发时间
    if (eraliestChanged) {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 重置timerfd
void TimerQueue::resetTimerfd(int timerfd_, Timestamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, '\0', sizeof(newValue));
    memset(&oldValue, '\0', sizeof(oldValue));

    // 剩下时间 = 超时时间 - 当前时间
    int64_t microSecondDif = expiration.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microSecondDif < 100) {
        microSecondDif = 100;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microSecondDif / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microSecondDif % Timestamp::kMicroSecondsPerSecond) * 1000);
    newValue.it_value = ts;
    // 此函数会唤醒事件循环
    if (::timerfd_settime(timerfd_, 0, &newValue, &oldValue)) {
        LOG_ERROR << "timerfd_settime faield()";
    }
}

void ReadTimerFd(int timerfd) 
{
    uint64_t read_byte;
    ssize_t readn = ::read(timerfd, &read_byte, sizeof(read_byte));
    
    if (readn != sizeof(read_byte)) {
        LOG_ERROR << "TimerQueue::ReadTimerFd read_size < 0";
    }
}

// 返回删除的定时器节点 （std::vector<Entry> expired）
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    // TODO:???
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    
    return expired;
}

// 处理到期定时器
// handleRead方法获取已经超时的定时器组数组，遍历到期的定时器并调用内部绑定的回调函数。之后调用reset方法重新设置定时器
void TimerQueue::handleRead()
{
    Timestamp now = Timestamp::now();
    ReadTimerFd(timerfd_);

    std::vector<Entry> expired = getExpired(now);

    // 遍历到期的定时器，调用回调函数
    callingExpiredTimers_ = true;
    for (const Entry& it : expired) {
        it.second->run();
    }
    callingExpiredTimers_ = false;
    
    // 重新设置这些定时器
    reset(expired, now);

}

// reset方法内部判断这些定时器是否是可重复使用的，如果是则继续插入定时器管理队列，之后自然会触发事件。
// 如果不是，那么就删除。如果重新插入了定时器，记得还需重置timerfd_。
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;
    for (const Entry& it : expired) {
        // 重复任务则继续执行
        if (it.second->repeat()) {
            auto timer = it.second;
            timer->restart(Timestamp::now());
            insert(timer);
        } else {
            delete it.second;
        }

        // 如果重新插入了定时器，需要继续重置timerfd
        if (!timers_.empty()) {
            resetTimerfd(timerfd_, (timers_.begin()->second)->expiration());
        }
    }
}

bool TimerQueue::insert(Timer* timer)
{
    bool earliestChanged = false;
    // 获取超时时间
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first) {
        // 说明最早的定时器已经被替换了
        earliestChanged = true;
    }

    // 定时器管理红黑树插入此新定时器
    timers_.insert(Entry(when, timer));

    return earliestChanged;
}


