#include "webserver.h"

WebServer::WebServer(
    int port, int trigMode, int timeoutMs, bool OptLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbName, int connPoolNum, int thhreadNum,
        bool openLog, int logLevel, int logQueSize):
        port_(port), openLinger_(OptLinger), timeoutMs_(timeoutMs), isClose_(false),
        timer_(new HeapTimer()), threadPool_(new ThreadPool(thhreadNum)), epoller_(new Epoller())
{
    /*
        port: 监听端口号
        trigMode: epoll的触发事件
        timeoutMS: 客户端连接超时时间
        OptLinger: 启用优雅关闭，即等待所有数据发送完毕
        sqlPort: 数据库端口号
        sqlUser: 数据库用户名
        sqlPwd: 数据库密码
        dbName: 数据库名称
        connPoolNum: 数据库连接池中连接的数量
        threadNum: 线程池中连接的数量
        openLog: 是否开启日志
        logLevel: 日志记录的级别
        logQueSize: 日志队列大小
    */
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    // std::cout << "srcDir: " << srcDir_ << "\n";
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    // std::cout << "conncation sql successful.\n";

    InitEventMode_(trigMode);
    if(!InitSocket_())
        isClose_ = true;

    // std::cout << "isClose: " << isClose_ << "\n";
    // std::cout << "openLog: " << openLog << "\n";

    if(openLog) {
        Log::Instance()->Init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) {
            LOG_ERROR("=============== Server Init error! ==================");
        }
        else {
            LOG_INFO("=========== Server init ==============");
            LOG_INFO("Port:%d, OpenLinger:%s", port_, OptLinger?"true":"false");
            LOG_INFO("Listen Mode:%s, OpenConn Mode:%s",
                        (listenEvent_ & EPOLLET?"ET":"LT"),
                        (connEvent_ & EPOLLET?"ET":"LT"));
            LOG_INFO("LogSys level:%d", logLevel);
            LOG_INFO("srcDir:%s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num:%d, ThreadPool num:%d", connPoolNum, thhreadNum);
        }


    }
}

void WebServer::Start() {
    int timeMS = -1;
    if(!isClose_)
        LOG_INFO("=============== Server start ==================");
    
    while(!isClose_) {
        if(timeoutMs_>0)
            timeMS = timer_->GetNextTick();
        
        int evenCnt = epoller_->wait(timeMS);
        for(int i=0; i<evenCnt; i++) {
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) {
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd)>0);
                CloseConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd)>0);
                DealRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd)>0);
                DealWrite_(&users_[fd]);
            }
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

/*
    使用 |= 运算符是为了将新的事件标志添加到现有的事件标志中，而不覆盖现有的标志。
    这种方式可以组合多个事件标志，使得一个事件可以同时具有多个触发条件。
*/
// 监听事件和连接事件的触发方式
void WebServer::InitEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;               // 表示对端关闭连接时触发事件
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;  // 表示对端关闭连接时触发事件
    switch(trigMode) {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;           // 设置为边缘触发
        case 2:
            connEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::SendError_(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn *client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeoutMs_ > 0 ) {
        timer_->add(fd, timeoutMs_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock_(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if(fd <= 0) {
            return ;
        }
        else if(HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Client is full!");
            return ;
        }

        AddClient_(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn *client) {
    assert(client);
    ExtentTime_(client);
    // threadPool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
    threadPool_->AddTask([this, client]() { this->OnRead_(client); });
}

void WebServer::DealWrite_(HttpConn *client) {
    assert(client);
    ExtentTime_(client);
    threadPool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn *client) {
    assert(client);
    if(timeoutMs_ > 0)
        timer_->adjust(client->GetFd(), timeoutMs_);
}

void WebServer::OnRead_(HttpConn *client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    }
    else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
} 

void WebServer::OnProcess_(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        if(client->IsKeepAlive()) {
            OnProcess_(client);
            return ;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return ;
        }
    }
    CloseConn_(client);
}

bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    
    if(port_ > 65535 || port_ < 1024) {         // 其中0到1023为特权端口或系统端口
        LOG_ERROR("Port:5d error!", port_);
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htonl(port_);
    struct linger optLinger = {0};
    if(openLinger_) {
        // 直到所剩数据发送或超时关闭
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        LOG_ERROR("Init linger error!");
        close(listenFd_);
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &optLinger, sizeof(optLinger));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error!");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("listen Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_, listenFd_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }

    SetFdNonblock_(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

int WebServer::SetFdNonblock_(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}