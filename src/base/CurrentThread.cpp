#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0) {
            // 调用 Linux 系统调用 SYS_gettid 获取当前线程的线程 ID（tid）值
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}