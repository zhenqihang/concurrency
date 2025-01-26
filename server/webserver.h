#pragma once

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/treadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"
#include "../timer/heaptimer.h"

class WebServer{
public:
    WebServer(
        int port, int trigMode, int timeoutMs, bool OptLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbName, int connPoolNum, int thhreadNum,
        bool openLog, int logLevel, int logQueSize);
    
    ~WebServer();
    void Start();
private:
    
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealRead_(HttpConn* client);
    void DealWrite_(HttpConn* client);

    void CloseConn_(HttpConn* client);
    void SendError_(int fd, const char* info);
    void ExtentTime_(HttpConn* client);
    
    void OnRead_(HttpConn* client);

    void OnProcess_(HttpConn* client);

    void OnWrite_(HttpConn* client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock_(int fd);

    int port_;
    bool openLinger_;
    int timeoutMs_;
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};

