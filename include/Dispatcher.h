#pragma once
#include "Channel.h"
#include <string>
using namespace std;

// 前向声明
class EventLoop;
// 事件分发器函数接口结构体
class Dispatcher
{
public:
    Dispatcher(EventLoop *evLoop);
    virtual ~Dispatcher();
    // 添加
    virtual int add();
    // 删除
    virtual int remove();
    // 修改
    virtual int modify();
    // 事件监测
    virtual int dispatch(int timeout = 2); // 单位: s
    inline void SetChannel(Channel *channel)
    {
        this->m_channel = channel;
    }

protected:
    string m_name = string();
    Channel *m_channel;
    EventLoop *m_evLoop;
};