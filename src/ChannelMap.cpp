#include "ChannelMap.h"

ChannelMap *ChannelMapInit(int size)
{
    ChannelMap *channelmap = (ChannelMap *)malloc(sizeof(ChannelMap));
    channelmap->size = size;
    // 创建一个指针数组，每一个指针指向一个Channel结构体
    channelmap->list = (Channel **)malloc(sizeof(Channel *) * size);
    memset(channelmap->list, 0, sizeof(Channel *) * size);
    return channelmap;
}

void ChannelMapClear(ChannelMap *map)
{
    if (map != nullptr)
    {
        for (int i = 0; i < map->size; i++)
        {
            if (map->list[i] != nullptr)
            {
                free(map->list[i]);
                map->list[i] = nullptr;
            }
        }
        free(map->list);
        map->list = nullptr;
    }
    map->size = 0;
}

bool makeMapRoom(ChannelMap *map, int newSize, int unitSize)
{
    if (map != nullptr)
    {
        if (map->size < newSize)
        {
            int currentSize = map->size;
            // 按2的倍数扩容，先找到扩完的大小，再进行扩容
            while (currentSize < newSize)
            {
                currentSize = currentSize * 2;
            }
            Channel **newMap = (Channel **)realloc(map->list, currentSize * unitSize);
            if (newMap != nullptr)
            {
                map->list = newMap;
                memset(&map->list[map->size], 0, (currentSize - map->size) * unitSize);
                map->size = currentSize;
                return true;
            }
        }
    }
    return false;
}
