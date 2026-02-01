#pragma once
#include <functional>

enum class EventType // 事件类型
{
    TimeOutEvent = 1,
    READ_EVENT = 2,
    WRITE_EVENT = 4
};
class Channel
{
public:
    using handleFunc = int (*)(void *);
    // using handleFunc = std::function<int(void *)>;
    // 事件回调函数
    handleFunc readHandler;
    handleFunc writeHandler;
    handleFunc destroyCallback;
    // Channel初始化
    Channel(int fd, EventType events, handleFunc readHandler, handleFunc writeHandler, handleFunc destroyHandler, void *arg);
    // 修改fd的写事件(检测 or 不检测)
    void writeEventEnable(bool flag);
    // 判断是否需要检测文件描述符的写事件
    bool isWriteEventEnable();
    inline int getfd() { return this->m_fd; }
    inline int getEvents() { return this->m_events; }
    inline const void *getArg() { return this->m_arg; }

private:
    int m_fd;     // 文件描述符
    int m_events; // 事件类型
    void *m_arg;  // 回调函数的参数
};
