#pragma once
#include <cstdlib>
#include <cstring>
#include <unistd.h>
using namespace std;
struct Buffer
{
    char *data; // 指向内存的指针
    int capacity;//容量
    int readPos;//读位置
    int writePos;//写位置
};
//初始化
Buffer* BufferInit(int Size);
//销毁buffer
void bufferDestroy(Buffer* buf);
// 当写入size大小的时候，进行条件性扩容
void bufferExtendRoom(struct Buffer* buffer, int size);
// 得到剩余的可直接写的内存容量
int bufferWriteableSize(struct Buffer* buffer);
// 得到未被读内存容量
int bufferReadableSize(struct Buffer* buffer);
// 往内存写数据
int bufferAppendData(struct Buffer* buffer, const char* data, int size);
int bufferAppendString(struct Buffer* buffer, const char* data);
// 接收套接字数据
int bufferSocketRead(struct Buffer* buffer, int fd);