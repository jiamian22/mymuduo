#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <vector>
#include <string>
#include <algorithm>

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size

// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    // prependable 初始大小，readIndex 初始位置，writeIndex 初始位置
    // 刚开始 readerIndex 和 writerIndex 处于同一位置
    static const size_t kCheapPrepend = 8;  // 头部预留8个字节
    static const size_t kInitialSize = 1024;  // 缓冲区初始化大小 1KB

    explicit Buffer(size_t initialSize = kInitialSize)
        :   buffer_(kCheapPrepend + initialSize),  // buffer分配大小 8 + 1KB
            readerIndex_(kCheapPrepend),  // 可读索引和可写索引最开始位置都在预留字节后
            writerIndex_(kCheapPrepend)
        {}
    
    /**
     * kCheapPrepend | reader | writer |
     * writerIndex_ - readerIndex_
     */
    // 可读空间大小
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    /**
     * kCheapPrepend | reader | writer |
     * buffer_.size() - writerIndex_
     */
    // 可写空间大小
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    /**
     * kCheapPrepend | reader | writer |
     * wreaderIndex_
     */
    // 预留空间大小
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    void retrieveUntil(const char* end)
    {
        retrieve(end - peek());
    }
    
    // onMessage string <- Buffer
    // 需要进行复位操作
    void retrieve(size_t len)
    {
        // 应用只读取可读缓冲区数据的一部分(读取了len的长度)
        if (len < readableBytes()) {
            // 移动可读缓冲区指针
            readerIndex_ += len;
        } else {  // 全部读完 len == readableBytes()
            retrieveAll();
        }
    }

    // 全部读完，则直接将可读缓冲区指针移动到写缓冲区指针那
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // DEBUG使用，提取出string类型，但是不会置位
    std::string GetBufferAllAsString()
    {
        size_t len = readableBytes();
        std::string result(peek(), len);
        return result;
    }

    // 将onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {   
        // 应用可读取数据的长度
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        // peek()可读数据的起始地址
        std::string result(peek(), len);
        // 上面一句把缓冲区中可读取的数据读取出来，所以要将缓冲区复位
        retrieve(len); 
        return result;
    }

    // 保证写空间足够len，如果不够则扩容
    // buffer_.size() - writeIndex_
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len) {
            // 扩容函数
            makeSpace(len);
        }
    }

    // string::data() 转换成字符数组，但是没有 '\0'
    void append(const std::string &str)
    {
        append(str.data(), str.size());
    }

    // void append(const char *data)
    // {
    //     append(data, sizeof(data));
    // }

    // 向buffer_添加数据 把[data, data+len]内存上的数据添加到缓冲区中
    void append(const char* data, size_t len)
    {
        // 确保可写空间足够
        ensureWritableBytes(len);
        // 将这段数据拷贝到可写位置之后
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    const char* findCRLF() const
    {
        // FIXME: replace with memmem()?
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int* saveErrno);
    
private:
    char* begin()
    {
        // 获取buffer_起始地址（迭代器类型要转成char*类型）
        return &(*buffer_.begin());
    }

    const char* begin() const
    {
        return &(*buffer_.begin());
    }

    // 扩容空间操作
    void makeSpace(int len)
    {
        /**
         * kCheapPrepend | reader | writer |
         * kCheapPrepend |       len         |
         */
        // 因为readIndex一直往后，之前的空间没有被利用，我们可以将后面数据复制到前面，如果挪位置都不够用，则只能重新分配buffer_大小
        // 整个buffer都不够用
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {  // 整个buffer够用，将后面移动到前面继续分配
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;  // 读取空间地址回归最开始状态
            writerIndex_ = readerIndex_ + readable;  // 写空间位置也前移了
        }
    }

    /**
     * 采取 vector 形式，可以自动分配内存，也可以提前预留空间大小
     */ 
    std::vector<char> buffer_;  // 缓冲区其实就是vector<char>
    size_t readerIndex_;  // 可读区域开始索引
    size_t writerIndex_;  // 可写区域开始索引
    static const char kCRLF[];
};

#endif // __BUFFER_H__