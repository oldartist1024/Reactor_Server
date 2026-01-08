#pragma once
// #include <iostream>
#include "Channel.h"
#include <string.h>
using namespace std;
struct ChannelMap
{
    int size;
    Channel** list;
};
ChannelMap* ChannelMapInit(int size);
// 清空map
void ChannelMapClear(ChannelMap* map);
// 重新分配内存空间
bool makeMapRoom(ChannelMap* map, int newSize, int unitSize);

