#include "log.h"

int main()
{
    Log::Instance()->Init(0, "./log", ".log", 1024);
    LOG_DEBUG("debug!");
    // LOG_INFO("========== Server init ==========");
    LOG_INFO("info!");
    LOG_WARN("warn!");
    LOG_ERROR("error!");
    return 0;
}