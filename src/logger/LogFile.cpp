#include "LogFile.h"

LogFile::LogFile(const std::string& basename,
        off_t rollSize,
        int flushInterval,
        int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(new std::mutex),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0)
{
    rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* data, int len)
{
    std::lock_guard<std::mutex> lock(*mutex_);
    appendInLock(data, len);
}

void LogFile::appendInLock(const char* data, int len)
{
    file_->append(data, len);

    if (file_->writtenBytes() > rollSize_) {
        // 如果当前日志文件写入的字节数超过了设定的滚动大小（rollSize_），则执行日志滚动操作
        rollFile();
    } else {
        // 如果当前日志文件写入的字节数未超过设定的滚动大小，则继续检查其他条件
        ++count_;  // 递增记录检查次数的计数器 count_
        if (count_ >= checkEveryN_) {
            // 如果 count_ 达到了设定的检查频率（checkEveryN_），则执行进一步的检查
            count_ = 0;  // 将计数器 count_ 重新置为 0，准备下一次检查
            time_t now = ::time(NULL);  // 获取当前时间戳（单位为秒），赋值给变量 now
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;  // 计算当前时间所属的周期时间赋值给 thisPeriod
            if (thisPeriod != startOfPeriod_) {
                // 如果当前时间的周期起点与日志开始记录时的周期起点不同，意味着跨越了一个时间周期（如跨天或跨小时），则执行日志滚动操作
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) {
                // 否则，如果自上次刷新日志以来的时间间隔超过了设定的刷新间隔（flushInterval_），则刷新日志
                lastFlush_ = now;  // 更新 lastFlush_，记录这次刷新的时间
                file_->flush();  // 调用文件对象的 flush 方法，将缓冲区中的内容写入磁盘
            }
        }
    }
}


void LogFile::flush()
{
    // std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
}

// 滚动日志，滚动日志是一种有效的日志管理技术，通过控制日志文件的大小和数量，确保系统稳定运行，方便日志分析，并减少对磁盘资源的消耗
// basename + time + hostname + pid + ".log"
bool LogFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    // 计算现在是第几天 now/kRollPerSeconds求出现在是第几天，再乘以秒数相当于是当前天数0点对应的秒数
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        // 让file_指向一个名为filename的文件，相当于新建了一个文件
        file_.reset(new FileUtil(filename));
        return true;
    }
    return false;
}

std::string LogFile::getLogFileName(const std::string& basename, time_t* now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now, &tm);
    // 写入时间
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
    filename += timebuf;

    filename += ".log";

    return filename;
}