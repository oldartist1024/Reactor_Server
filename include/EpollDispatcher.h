#pragma once
#include <unistd.h>
#include "Dispatcher.h"
#include <sys/epoll.h>
using namespace std;
class EpollDispatcher : public Dispatcher
{
public:
    EpollDispatcher(EventLoop *evLoop);
    ~EpollDispatcher();
    // 添加
    int add() override;
    // 删除
    int remove() override;
    // 修改
    int modify() override;
    // 事件监测
    int dispatch(int timeout = 2) override; // 单位: s
private:
    int epollctl(int op);

private:
    int m_epfd;
    epoll_event *m_event;
    const int m_maxNode = 520;
};