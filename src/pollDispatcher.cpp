#include "pollDispatcher.h"

PollDispatcher::PollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    m_maxfd = 0;
    m_fd = new pollfd[m_maxNode];
    for (int i = 0; i < m_maxNode; i++)
    {
        m_fd[i].fd = -1;
        m_fd[i].events = 0;
        m_fd[i].revents = 0;
    }
    m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
    delete[] m_fd;
}

int PollDispatcher::add()
{
    int ret = -1;
    int events = 0;
    if (m_channel->getEvents() & (int)EventType::READ_EVENT)
        events |= POLLIN;
    if (m_channel->getEvents() & (int)EventType::WRITE_EVENT)
        events |= POLLOUT;
    for (int i = 0; i < m_maxNode; i++)
    {
        if (m_fd[i].fd == -1)
        {
            m_fd[i].fd = m_channel->getfd();
            m_fd[i].events = events;
            if (m_maxfd < i)
                m_maxfd = i;
            ret = 1;
            break;
        }
    }
    return ret;
}

int PollDispatcher::remove()
{
    int ret = -1;
    for (int i = 0; i < m_maxNode; i++)
    {
        if (m_fd[i].fd == m_channel->getfd())
        {
            m_fd[i].fd = -1;
            m_fd[i].events = 0;
            m_fd[i].revents = 0;
            // 要不要更新maxfd
            ret = 1;
            break;
        }
    }
    // 移除channel后，调用其销毁回调函数（断开tcp连接的函数）
    m_channel->destroyCallback(const_cast<void *>(m_channel->getArg()));
    return ret;
}

int PollDispatcher::modify()
{
    int ret = -1;
    int events = 0;
    if (m_channel->getEvents() & (int)EventType::READ_EVENT)
        events |= POLLIN;
    if (m_channel->getEvents() & (int)EventType::WRITE_EVENT)
        events |= POLLOUT;
    for (int i = 0; i < m_maxNode; i++)
    {
        if (m_fd[i].fd == m_channel->getfd())
        {
            m_fd[i].events = events;
            ret = 1;
            break;
        }
    }
    return ret;
}

int PollDispatcher::dispatch(int timeout)
{
    int res = poll(m_fd, m_maxfd + 1, timeout * 1000);
    if (res == -1)
    {
        perror("poll");
        exit(0);
    }
    for (int i = 0; i < m_maxfd + 1; i++)
    {
        int fd = m_fd[i].fd;
        if (fd == -1)
            continue;
        int events = m_fd[i].revents;
        if (events & POLLERR || events & POLLHUP)
        {
            // 对方断开了连接, 删除 fd
            continue;
        }
        if (events & POLLIN)
        {
            m_evLoop->Activate(fd, (int)EventType::READ_EVENT);
        }
        if (events & POLLOUT)
        {
            m_evLoop->Activate(fd, (int)EventType::WRITE_EVENT);
        }
    }
    return 0;
}
