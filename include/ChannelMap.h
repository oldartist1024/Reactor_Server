#pragma once
// #include <iostream>
#include "Channel.h"
#include <string.h>
using namespace std;
struct ChannelMap
{
    int size;       // 大小
    Channel **list; // 指针数组
};
// ChannelMap 初始化
ChannelMap *ChannelMapInit(int size);
// ChannelMap 清空
void ChannelMapClear(ChannelMap *map);
// ChannelMap 扩容
bool makeMapRoom(ChannelMap *map, int newSize, int unitSize);
