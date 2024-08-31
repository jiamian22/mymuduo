#ifndef __INET_ADDRESS_H__
#define __INET_ADDRESS_H__

#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");  // 指定端口号和IP地址，默认IP地址为 "127.0.0.1"
    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) { }  // 接受一个 sockaddr_in 结构体作为参数

    // 返回该网络地址的IP地址部分，以字符串形式返回
    std::string toIp() const;
    // 返回包含IP地址和端口号的字符串表示
    std::string toIpPort() const;
    // 返回该网络地址的端口号
    uint16_t toPort() const;

    // 返回指向内部 sockaddr_in 结构体的常量指针，用于获取底层网络地址结构
    const sockaddr_in* getSockAddr() const { return &addr_; }
    // 设置内部的 sockaddr_in 结构体，用给定的 sockaddr_in 结构体更新当前网络地址信息
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};

#endif // __INET_ADDRESS_H__