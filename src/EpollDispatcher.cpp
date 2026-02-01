#include "EpollDispatcher.h"
#include "EventLoop.h"
EpollDispatcher::EpollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    // 创建epoll树
    m_epfd = epoll_create(1);
    // 创建失败
    if (m_epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    m_event = new epoll_event[m_maxNode];
    m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
    close(m_epfd);
    delete[] m_event;
}

int EpollDispatcher::add()
{
    int ret = epollctl(EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_ctl add");
        return 0;
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = epollctl(EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_ctl del");
        return 0;
    }
    // 移除channel后，调用其销毁回调函数（断开tcp连接的函数）
    m_channel->destroyCallback(const_cast<void *>(m_channel->getArg()));
    return ret;
}

int EpollDispatcher::modify()
{
    int ret = epollctl(EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_ctl mod");
        return 0;
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
    int num = epoll_wait(m_epfd, m_event, m_maxNode, timeout * 1000);
    for (int i = 0; i < num; i++)
    {
        int events = m_event[i].events;
        int fd = m_event[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // 对方断开了连接, 删除 fd
            continue;
        }
        if (events & EPOLLIN)
        {
            m_evLoop->Activate(fd, (int)EventType::READ_EVENT);
        }
        if (events & EPOLLOUT)
        {
            m_evLoop->Activate(fd, (int)EventType::WRITE_EVENT);
        }
    }
    return 0;
}

int EpollDispatcher::epollctl(int op)
{
    epoll_event ev;
    // 事件类型设置
    int events = 0;
    if (m_channel->getEvents() & (int)EventType::READ_EVENT)
        events |= EPOLLIN;
    if (m_channel->getEvents() & (int)EventType::WRITE_EVENT)
        events |= EPOLLOUT;
    ev.events = events;
    // 文件描述符设置
    ev.data.fd = m_channel->getfd();
    // 对epoll树进行操作
    int ret = epoll_ctl(m_epfd, op, m_channel->getfd(), &ev);
    return ret;
}
