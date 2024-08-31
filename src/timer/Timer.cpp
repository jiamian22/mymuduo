#include "Timer.h"

void Timer::restart(Timestamp now)
{
    if (repeat_) {
        // 如果是重复定时事件，则继续添加定时事件，得到新事件到期事件
        expiration_ = addTime(now, interval_);  // 设置其下一次超时时间为「当前时间 + 间隔时间」
    } else {
        // 如果是「一次性定时器」，那么就会自动生成一个空的 Timestamp，其时间自动设置为 0.0
        expiration_ = Timestamp();
    }
}