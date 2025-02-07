#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
    Log();
    void AppendLogLevelTitle_(int level);
    virtual ~Log();                      // 为确保调用多态时正确调用析构函数
    void AsyncWrite_();                  // 异步写入文件

    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;                  // 文件路径
    const char* suffix_;                // 文件后缀名

    int MAX_LINES_;
    int lineCount_;
    int toDay_;
    bool isOpen_;
    Buffer buff_;
    int level_;
    bool isAsync_;                      // 是否异步

    FILE* fp_;                          // 文件描述符
    std::unique_ptr<BlockDeque<std::string>> deque_;    // 用于存储日志信息的阻塞队列
    std::unique_ptr<std::thread> writeThread_;          // 异步线程
    std::mutex mtx_;

public:
    void Init(int level, const char* path = "./log",
                const char* suffix = ".log",
                int maxQueueCapacity = 1024);
    
    static Log* Instance();
    static void FlushLogThread();

    void write(int level, const char *format, ...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }

};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);
