#ifndef __MYSQL_CONNECTION_POOL_H__
#define __MYSQL_CONNECTION_POOL_H__

#include "MysqlConn.h"
#include "json.hpp"
using json = nlohmann::json; 

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

// 实现数据库连接池功能模块
class ConnectionPool
{
public:
    // 获取连接池对象实例
    static ConnectionPool* getConnectionPool();
    // 给外部提供接口，从连接池中获取一个可用的空闲连接
    std::shared_ptr<MysqlConn> getConnection();
    ~ConnectionPool();

private:
    ConnectionPool();  // 单例，构造函数私有化
    ConnectionPool(const ConnectionPool& obj) = delete;  // 单例，删除拷贝构造
    ConnectionPool(const ConnectionPool&& obj) = delete;  // 单例，删除移动构造
    ConnectionPool& operator=(const ConnectionPool& obj) = delete;  // 单例，删除拷贝赋值

    // 从配置文件中加载配置项 loadConfigFile
    bool parseJsonFile();
    // 运行在独立的线程中，专门负责生产新连接
    void produceConnection();
    // 运行在独立的线程中，专门负责消费新连接
    void recycleConnection();
    void addConnection();

    // TODO:加上文件路径
    // std::string filePath_;
    std::string ip_;  // MySQL的ip地址
    std::string user_;  // MySQL的登录用户名
    std::string passwd_;  // MySQL的登录密码
    std::string dbName_;  // MySQL的数据库名称
    unsigned short port_;  // MySQL的端口号，默认3306
    int minSize_;  // 连接池的初始连接量
    int maxSize_;  // 连接池的最大连接量
    int currentSize_;
    // atomic int connectionCnt_;  // 记录连接所创建的connection连接的总数量
    int timeout_;  // 连接池获取连接的超时时间
    int maxIdleTime_;  // 连接池最大空闲时间
    std::queue<MysqlConn*> connectionQueue_;  // 连接池队列，存储MySQL的连接
    std::mutex mutex_;  // 维护连接队列的线程安全互斥锁
    std::condition_variable cond_;  //设置条件变量，用于连接生产线程和连接消费线程的通信
};

#endif  // __MYSQL_CONNECTION_POOL_H__