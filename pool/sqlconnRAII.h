#pragma once

#include "sqlconnpool.h"

class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connPool) {
        assert(connPool);
        *sql = connPool->GetConn();
        sql_ = *sql;
        connPool_ = connPool;
    }

    ~SqlConnRAII() {
        if(sql_)    
            connPool_->FreeConn(sql_);
    }

private:
    MYSQL *sql_;
    SqlConnPool* connPool_;
};