#pragma once
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <string>
using namespace std;
class Buffer
{
public:
    // 初始化
    Buffer(int Size);
    // 销毁buffer
    ~Buffer();
    // 当写入size大小的时候，进行选择性扩容
    void ExtendRoom(int size);
    // 得到剩余的可直接写的内存容量
    int WriteableSize();
    // 得到未被读的内存容量
    int ReadableSize();
    // 往内存写数据
    int AppendString(const char *data, int size);
    int AppendString(const char *data);
    int AppendString(const string data);
    // 接收套接字数据
    int SocketRead(int fd);
    // 解析请求头找到\r\n的位置
    char *FindCRLF();
    // 发送数据
    int SendData(int cfd);
    inline char *data_position()
    {
        return m_data + m_readPos;
    }
    inline int readPosIncrease(int count)
    {
        m_readPos += count;
        return m_readPos;
    }

private:
    char *m_data;       // 指向内存的指针
    int m_capacity;     // 容量
    int m_readPos = 0;  // 读位置
    int m_writePos = 0; // 写位置
};
