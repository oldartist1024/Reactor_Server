#include "Buffer.h"

Buffer *BufferInit(int Size)
{
    Buffer *buffer = (Buffer *)malloc(sizeof(Buffer));
    if (buffer)
    {
        buffer->data = (char *)malloc(Size);
        buffer->capacity = Size;
        buffer->readPos = 0;
        buffer->writePos = 0;
        memset(buffer->data, 0, Size);
    }
    return buffer;
}

void bufferDestroy(Buffer *buf)
{
    if (buf)
    {
        if (buf->data)
            free(buf->data);
        free(buf);
    }
}

void bufferExtendRoom(Buffer *buffer, int size)
{
    if (bufferWriteableSize(buffer) >= size)
    {
        // 1.能直接写
        return;
    }
    else if (buffer->capacity - bufferReadableSize(buffer) >= size)
    {
        // 2.可以移动一下内存再写
        int readablesize = bufferReadableSize(buffer);
        memcpy(buffer->data, buffer->data + buffer->readPos, readablesize);
        buffer->readPos = 0;
        buffer->writePos = readablesize;
        memset(buffer->data + readablesize, 0, buffer->capacity - readablesize);
    }
    else
    {
        // 3.写不下，进行扩容
        char *temp = (char *)realloc(buffer->data, buffer->capacity + size);
        if (temp == NULL)
        {
            return; // 申请失败了
        }
        memset(temp + buffer->capacity, 0, size);
        // 更新数据
        buffer->data = temp;
        buffer->capacity += size;
    }
}

int bufferWriteableSize(Buffer *buffer)
{
    return buffer->capacity - buffer->writePos;
}

int bufferReadableSize(Buffer *buffer)
{
    return buffer->writePos - buffer->readPos;
}

int bufferAppendData(Buffer *buffer, const char *data, int size)
{
    // 检测内存以及数据是否为空
    if (!buffer || !data || size <= 0)
        return -1;
    // 扩容确保有足够的空间写入数据
    bufferExtendRoom(buffer, size);
    memcpy(buffer->data + buffer->writePos, data, size);
    buffer->writePos += size;
    return size;
}

int bufferAppendString(Buffer *buffer, const char *data)
{
    // 追加字符串
    bufferAppendData(buffer, data, strlen(data));
    return strlen(data);
}

int bufferSocketRead(Buffer *buffer, int fd)
{
    char buf[1024];
    // 从套接字读取数据
    int res = read(fd, buf, sizeof(buf));
    if (res < 0) // 读取失败
        return -1;
    else
    {
        // 读取成功，追加到缓冲区
        bufferAppendData(buffer, buf, res);
    }
    return res;
}

char *bufferFindCRLF(Buffer *buffer)
{
    // 在可读数据中查找\r\n，找到返回指针，指针指向\r
    char *ptr = (char *)memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
    return ptr;
}

int bufferSendData(Buffer *buffer, int cfd)
{
    // 获取还未被读的数据长度
    int readableSize = bufferReadableSize(buffer);
    if (readableSize > 0)
    {
        // 将可读数据发送到套接字
        // 加MSG_NOSIGNAL，防止发送数据时，对端关闭连接导致的SIGPIPE信号
        int count = send(cfd, buffer->data + buffer->readPos, readableSize, MSG_NOSIGNAL);
        if (count > 0)
        {
            buffer->readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}