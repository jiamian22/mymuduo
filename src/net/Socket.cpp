#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "Socket.h"
#include "Logging.h"
#include "InetAddress.h"

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in))) {
        LOG_FATAL << "bind sockfd:" << sockfd_ << " fail";
    }
}

void Socket::listen(int nn)
{
    if (0 != ::listen(sockfd_, nn)) {
        LOG_FATAL << "listen sockfd:" << sockfd_ << " fail";
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     **/
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    // 从监听套接字 sockfd 中接受一个连接，并返回一个新的套接字描述符用于通信。如果 addr 不为 NULL，则将客户端的地址信息填充到 addr 中
    // int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    // accept4 与 accept 类似，从监听套接字 sockfd 中接受一个连接，并支持设置额外的 flags
    // 可以通过 flags 参数设置非阻塞 (SOCK_NONBLOCK) 和关闭执行 (SOCK_CLOEXEC) 标志，以及其他一些标志，而不需要额外的系统调用来设置这些属性
    // 在高并发的网络编程中，accept4 可以减少额外的系统调用，提高效率，特别是在多线程或多进程服务器中
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);  // 非阻塞版本的accept
    
    if (connfd >= 0) {
        peeraddr->setSockAddr(addr);  // 客户端的地址和协议
    } else {
        LOG_ERROR << "accept4() failed";
    }
    return connfd;
}

// TODO:socket的各个设置的用法
void Socket::shutdownWrite()
{
    // 关闭写端，但是可以接受客户端数据
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        LOG_ERROR << "shutdownWrite error";
    }
}

// 不启动Nagle算法
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

// 设置地址复用，其实就是可以使用处于Time-wait的端口
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

// 通过改变内核信息，多个进程可以绑定同一个地址。通俗就是多个服务的ip+port是一样
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}