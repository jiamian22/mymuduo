#include "InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    // 使用 bzero 函数将 addr_ 的内存清零，确保 addr_ 结构体的所有字段都初始化为0
    ::bzero(&addr_, sizeof(addr_));
    // 设置地址族为 IPv4
    addr_.sin_family = AF_INET;
    // 设置端口号，并使用 htons 函数将主机字节序转换为网络字节序（大端字节序）
    addr_.sin_port = ::htons(port);
    // 设置IP地址。当前使用的是 INADDR_ANY，表示接受任意IP地址，这在服务器端常用于绑定到所有可用的网络接口上
    // inet_addr() 用于将点分十进制的 IP 地址字符串转换为网络字节序的整数，而 htons() 用于将主机字节序的整数转换为网络字节序的整数
    // addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());  // 只能针对特定的网络接口，不够灵活
    addr_.sin_addr.s_addr = htonl(INADDR_ANY); // FIXME : it should be addr.ip() and need some conversion
}

std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    // 将存储在 addr_.sin_addr 中的 IPv4 地址转换为可读的字符串格式，并存储在 buf 中
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    // 将存储在 addr_.sin_addr 中的 IPv4 地址转换为可读的字符串格式，并存储在 buf 中
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = ::strlen(buf);
    // 将端口号从网络字节序（大端字节序）转换为主机字节序
    uint16_t port = ::ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);  // 将端口号添加到 IP 地址字符串的末尾
    return buf;
}

uint16_t InetAddress::toPort() const
{
    // 将存储在 addr_.sin_port 中的端口号从网络字节序（大端字节序）转换为主机字节序，并返回该端口号
    return ::ntohs(addr_.sin_port);
}

// TODO: 网络字节序和主机字节序的转换函数

#if 0
#include <iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;
}
#endif