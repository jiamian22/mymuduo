/*
实现异步日志需要先将前端消息储存起来，然后到达一定数量或者一定时间再将这些信息写入磁盘。
muduo 使用 FixedBuffer 类来实现这个存放日志信息的缓冲区。
*/

#ifndef __FIXED_BUFFER_H__
#define __FIXED_BUFFER_H__

#include <assert.h>
#include <string.h> // memcpy
#include <strings.h>
#include <string>

#include "noncopyable.h"

// 针对不同的缓冲区，设置两个固定容量
// LogStream 类用于重载正文信息，一次信息大小是有限的，其使用的缓冲区的大小就是 kSmallBuffer
// 后端日志 AsyncLogging 需要存储大量日志信息，其会使用的缓冲区大小更大，所以是 kLargeBuffer
const int kSmallBuffer = 4000;  // 4KB
const int kLargeBuffer = 4000 * 1000;  // 4MB

template <int SIZE>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer() : cur_(data_) { }

    // 向缓冲区添加数据
    void append(const char* buf, size_t len)
    {
        if (static_cast<size_t>(avail()) > len) {
            // 将buf地址，长为len的数据拷贝到cur_地址处
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(end() - data_); }

    // write to data_ directly
    char* current() { return cur_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }
    void bzero() { ::bzero(data_, sizeof(data_)); }

    // 将从 data_ 指向的内存区域开始、长度为 length() 的字符序列转换成一个 std::string 对象
    std::string toString() const { return std::string(data_, length()); }

private:
    const char* end() const { return data_ + sizeof(data_); }

    char data_[SIZE];
    char* cur_; 
};

#endif // __FIXED_BUFFER_H__