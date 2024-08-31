/*
定义了一个工具类 CurrentThread，封装了获取当前线程 ID 的功能。
通过缓存 t_cachedTid 变量，在需要时获取并返回当前线程的 tid，避免了重复的系统调用，提高了性能和效率。
*/
#ifndef __CURRENT_THREAD_H__
#define __CURRENT_THREAD_H__

#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread
{
    // 使用 __thread 关键字声明的 t_cachedTid 是线程局部存储的，每个线程拥有自己的 t_cachedTid 变量，互不干扰，
    // 因此在多线程环境下不会出现竞争条件，保证了线程安全性
    extern __thread int t_cachedTid;  // 保存tid缓冲，避免多次系统调用
    
    void cacheTid();

    // 内联函数
    inline int tid()
    {
        // __builtin_expect(expression, expected_value) 的作用在于给编译器提供关于表达式的预期结果的提示，从而帮助编译器生成更高效的代码。它并不改变程序的实际逻辑或控制流程
        // __builtin_expect(t_cachedTid == 0, 0)表示编译器期望 t_cachedTid == 0 的结果是 false
        // __builtin_expect 不影响程序的逻辑执行，尽管 expected_value 设置为 0 表示我们期望条件为 false，但实际执行取决于 expression 的计算结果
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }
}

#endif // __CURRENT_THREAD_H__