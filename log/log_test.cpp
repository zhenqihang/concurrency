#include "log.h"

int port_ = 8888, OptLinger = 1;


int main()
{
    Log::Instance()->Init(0, "./log", ".log", 1024);

    // LOG_INFO("log_info");
    LOG_INFO("=========== Server init ==============");
    LOG_INFO("Port:%d, OpenLinger:%s", port_, OptLinger?"true":"false");
    // LOG_INFO("Listen Mode:%s, OpenConn Mode:%s",
    //             (listenEvent_ & EPOLLET?"ET":"LT"),
    //             (connEvent_ & EPOLLET?"ET":"LT"));
    // LOG_INFO("LogSys level:%d", logLevel);
    // LOG_INFO("srcDir:%s", HttpConn::srcDir);
    // LOG_INFO("SqlConnPool num:%d, ThreadPool num:%d", connPoolNum, thhreadNum);
    
    return 0;
}