cmake_minimum_required(VERSION 3.10)
project(WebServer)

# 查找pthread库
find_package(Threads REQUIRED)

# 查找mysql客户端库
find_library(MYSQLCLIENT_LIB mysqlclient)

include_directories(${PROJECT_SOURCE_DIR}/server)
include_directories(${PROJECT_SOURCE_DIR}/buffer)
include_directories(${PROJECT_SOURCE_DIR}/log)
include_directories(${PROJECT_SOURCE_DIR}/pool)
include_directories(${PROJECT_SOURCE_DIR}/http)
include_directories(${PROJECT_SOURCE_DIR}/timer)

set(SOURCES
    main.cpp
    buffer/buffer.cpp
    http/httpconn.cpp
    http/httprequest.cpp
    http/httpresponse.cpp
    log/blockqueue.h
    log/log.cpp
    pool/sqlconnpool.cpp
    pool/sqlconnRAII.h
    pool/treadpool.h
    server/webserver.cpp
    server/epoller.cpp
    timer/heaptimer.cpp
)

# 添加可执行文件
add_executable(webserver ${SOURCES})

target_link_libraries(webserver ${CMAKE_THREAD_LIBS_INIT} ${MYSQLCLIENT_LIB})