#include "Buffer.h"
Buffer::Buffer(int Size) : m_capacity(Size)
{

    m_data = (char *)malloc(Size);
    memset(m_data, 0, Size);
}

Buffer::~Buffer()
{
    if (m_data)
        free(m_data);
}

void Buffer::ExtendRoom(int size)
{
    if (WriteableSize() >= size)
    {
        // 1.能直接写
        return;
    }
    else if (m_capacity - m_writePos >= size)
    {
        // 2.可以移动一下内存再写
        int readablesize = ReadableSize();
        memcpy(m_data, m_data + m_readPos, readablesize);
        m_readPos = 0;
        m_writePos = readablesize;
        memset(m_data + readablesize, 0, m_capacity - readablesize);
    }
    else
    {
        // 3.写不下，进行扩容
        char *temp = (char *)realloc(m_data, m_capacity + size);
        if (temp == NULL)
        {
            return; // 申请失败了
        }
        memset(temp + m_capacity, 0, size);
        // 更新数据
        m_data = temp;
        m_capacity += size;
    }
}

int Buffer::WriteableSize()
{
    return m_capacity - m_writePos;
}

int Buffer::ReadableSize()
{
    return m_writePos - m_readPos;
}

int Buffer::AppendString(const char *data, int size)
{
    // 检测内存以及数据是否为空
    if (!data || size <= 0)
        return -1;
    // 扩容确保有足够的空间写入数据
    ExtendRoom(size);
    memcpy(m_data + m_writePos, data, size);
    m_writePos += size;
    return size;
}

int Buffer::AppendString(const char *data)
{
    AppendString(data, strlen(data));
    return strlen(data);
}

int Buffer::AppendString(const string data)
{
    int ret = AppendString(data.data());
    return ret;
}

int Buffer::SocketRead(int fd)
{
    char buf[1024];
    // 从套接字读取数据
    int res = read(fd, buf, sizeof(buf));
    if (res < 0) // 读取失败
        return -1;
    else
    {
        // 读取成功，追加到缓冲区
        AppendString(buf, res);
    }
    return res;
}
int Buffer::SendData(int cfd)
{
    // 获取还未被读的数据长度
    int readableSize = ReadableSize();
    if (readableSize > 0)
    {
        // 将可读数据发送到套接字
        // 加MSG_NOSIGNAL，防止发送数据时，对端关闭连接导致的SIGPIPE信号
        int count = send(cfd, m_data + m_readPos, readableSize, MSG_NOSIGNAL);
        if (count > 0)
        {
            m_readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
char *Buffer::FindCRLF()
{
    // 在可读数据中查找\r\n，找到返回指针，指针指向\r
    char *ptr = (char *)memmem(m_data + m_readPos, ReadableSize(), "\r\n", 2);
    return ptr;
}
