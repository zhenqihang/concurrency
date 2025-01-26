#pragma once
#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <assert.h>
#include "../log/log.h"

class SqlConnPool {
public:
    static SqlConnPool *Instance();
    MYSQL* GetConn();
    void FreeConn(MYSQL* conn);
    int GetFreeCount();

    void Init(const char* host, int port,
            const char* user, const char* pwd,
            const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

    int MAX_CONN_;
    int useCount_;
    int freeCount_;
    std::queue<MYSQL*> connQue_;
    std::mutex mtx_;
    sem_t semId_;                 // 信号量，管理数据库连接池
};
