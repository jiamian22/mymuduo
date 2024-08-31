#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) { }
    ~Socket();

    // 获取sockfd
    int fd() const { return sockfd_; }
    // 绑定sockfd，调用bind绑定服务器IP和端口
    void bindAddress(const InetAddress &localaddr);
    // 使sockfd为可接受连接状态，调用listen监听套接字
    void listen(int nn = 1024);
    // 接受连接，调用accept接收新客户连接请求 只不过这里封装的是accept4
    int accept(InetAddress* peeraddr);

    // 设置半关闭（调用shutdown关闭服务端写通道）
    void shutdownWrite();

    /* 下面四个函数都是调用setsockopt来设置一些socket选项 */
    void setTcpNoDelay(bool on);  // 设置Nagel算法，不启动naggle算法 增大对小数据包的支持
    void setReuseAddr(bool on);  // 设置地址复用
    void setReusePort(bool on);  // 设置端口复用
    void setKeepAlive(bool on);  // 设置长连接

private:
    const int sockfd_;  // Socket持有的fd，在构造函数中传进来
};

#endif // __SOCKET_H__