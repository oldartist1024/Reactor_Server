#pragma once
#include "iostream"
#include "Buffer.h"
#include "string"
#define ResHeaderSize 16

// 定义状态码枚举
enum HttpStatusCode
{
    Unknown = 0,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};
// 响应头结构体
struct ResponseHeader
{
    char key[64];
    char value[256];
};
using responseBody = void (*)(const char *fileName, struct Buffer *sendBuf, int socket);
// http响应结构体
struct HttpResponse
{
    enum HttpStatusCode statusCode; // 状态码
    char statusMessage[64];         // 状态消息
    char filename[256];             // 响应的文件名或者目录名
    ResponseHeader *header;         // 响应头数组
    int headerCount;                // 响应头数量
    responseBody sendDataFunc;      // 发送响应体的函数指针
};
// 初始化
HttpResponse *HttpResponseInit(void);
// 销毁
void httpResponseDestroy(HttpResponse *response);
// 添加响应头
void httpResponseAddHeader(HttpResponse *response, const char *key, const char *value);
// 将响应数据写入WriteBuffer
void httpResponsePrepareMsg(HttpResponse *response, Buffer *sendBuf, int socket);
