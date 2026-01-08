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
    // 1.能直接写
    if (bufferWriteableSize(buffer) >= size)
    {

    }
    // 2.可以移动一下内存再写
    else if (buffer->capacity - bufferReadableSize(buffer) >= size)
    {
        int readablesize = bufferReadableSize(buffer);
        memcpy(buffer->data, buffer->data + buffer->readPos, readablesize);
        buffer->readPos = 0;
        buffer->writePos = readablesize;
        memset(buffer->data + readablesize, 0, buffer->capacity - readablesize);
    } // 3.写不下，进行扩容
    else
    {
        char* temp = (char*)realloc(buffer->data, buffer->capacity + size);
        if (temp == NULL)
        {
            return; // 失败了
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
    if(!buffer||!data||size<=0)
        return -1;
    bufferExtendRoom(buffer, size);
    memcpy(buffer->data + buffer->writePos, data, size);
    buffer->writePos += size;
    return size;
}

int bufferAppendString(Buffer *buffer, const char *data)
{
    bufferAppendData(buffer, data, strlen(data));
    return strlen(data);
}

int bufferSocketRead(Buffer *buffer, int fd)
{
    char buf[1024];
    int res= read(fd,buf, sizeof(buf));
    if(res<0)
        return -1;
    else
    {
        bufferAppendData(buffer,buf,res);
    }
    return res;
}
