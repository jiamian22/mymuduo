

/*
基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程
*/



#include <iostream>
#include <functional>

#include "EventLoop.h"  // #include <mymuduo/EventLoop.h>
#include "TcpServer.h"  // #include <mymuduo/TcpServer.h>
#include "Logging.h"  // #include <mymuduo/Logging.h>
#include "AsyncLogging.h"  // #include <mymuduo/AsyncLogging.h>

class EchoServer
{
public:
    // 事件循环、IP+Port、服务器的名字
    EchoServer(EventLoop* loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 给服务器注册用户连接的创建和断开回调。将用户定义的连接事件处理函数注册进TcpServer中，TcpServer发生连接事件时会执行onConnection函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        
        // 给服务器注册用户读写事件回调。将用户定义的可读事件处理函数注册进TcpServer中，TcpServer发生可读事件时会执行onMessage函数
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置服务器端合适的subloop线程数量（1个I/O线程、3个worker线程）
        server_.setThreadNum(4);
    }

    // 开启服务器监听
    void start()
    {
        server_.start();
    }

private:
    // 连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected()) {
            LOG_INFO << "Connection UP : " << conn->peerAddress().toIpPort().c_str();
        } else {
            LOG_INFO << "Connection DOWN : " << conn->peerAddress().toIpPort().c_str();
            // conn->shutdown();  // close(fd)
            // loop_->quit();
        }
    }

    // 可读写事件回调，参数分别为：连接、缓冲区、接收到数据的时间信息
    void onMessage(const TcpConnectionPtr &conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, " << "data received at " << time.toFormattedString();
        conn->send(msg);
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }

    EventLoop* loop_;  // epoll
    TcpServer server_;
};


int main() 
{

    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    InetAddress addr(8080);  // InetAddress其实是对socket编程中的sockaddr_in进行封装，使其变为更友好简单的接口而已
    EchoServer server(&loop, addr, "EchoServer");

    server.start();  // 启动服务，将listenfd通过epoll_ctl添加到epoll
    loop.loop();  // 启动EventLoop::loop()函数，epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等

    return 0;
}