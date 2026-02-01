#include "selectDispatcher.h"
SelectDispatcher::SelectDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    FD_ZERO(&m_readfds);
    FD_ZERO(&m_writefds);
    m_name = "select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
    if (m_channel->getfd() >= m_maxNode)
        return -1;
    setFdSet();
    return 1;
}

int SelectDispatcher::remove()
{
    if (m_channel->getfd() >= m_maxNode)
        return -1;
    clearFdSet();
    m_channel->destroyCallback(const_cast<void *>(m_channel->getArg()));
    return 1;
}

int SelectDispatcher::modify()
{
    if (m_channel->getfd() >= m_maxNode)
        return -1;
    clearFdSet();
    setFdSet();
    return 1;
}

int SelectDispatcher::dispatch(int timeout)
{
    // 检测时间定义
    timeval val;
    val.tv_sec = timeout; /* Seconds.  */
    val.tv_usec = 0;      /* Microseconds.  */
    // 暂存检测集合
    fd_set rdtemp = m_readfds;
    fd_set wrtemp = m_writefds;
    // 不动原集合，只检测副本
    int res = select(m_maxNode, &rdtemp, &wrtemp, NULL, &val);
    if (res == -1)
    {
        perror("select");
        exit(0);
    }
    for (int i = 0; i < m_maxNode; i++)
    {
        if (FD_ISSET(i, &rdtemp))
        {
            // 读事件
            m_evLoop->Activate(i, (int)EventType::READ_EVENT);
        }
        if (FD_ISSET(i, &wrtemp))
        {
            // 写事件
            m_evLoop->Activate(i, (int)EventType::WRITE_EVENT);
        }
    }
    return 0;
}

void SelectDispatcher::setFdSet()
{
    if (m_channel->getEvents() & (int)EventType::READ_EVENT)
        FD_SET(m_channel->getfd(), &m_readfds);
    if (m_channel->getEvents() & (int)EventType::WRITE_EVENT)
        FD_SET(m_channel->getfd(), &m_writefds);
}

void SelectDispatcher::clearFdSet()
{
    if (m_channel->getEvents() & (int)EventType::READ_EVENT)
        FD_CLR(m_channel->getfd(), &m_readfds);
    if (m_channel->getEvents() & (int)EventType::WRITE_EVENT)
        FD_CLR(m_channel->getfd(), &m_writefds);
}
