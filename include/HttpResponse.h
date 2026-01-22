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
struct ResponseHeader
{
    char key[64];
    char value[256];
};
using responseBody = void (*)(const char *fileName, struct Buffer *sendBuf, int socket);
struct HttpResponse
{
    enum HttpStatusCode statusCode;
    char statusMessage[64];
    char filename[256];
    ResponseHeader *header;
    int headerCount;
    responseBody sendDataFunc;
};
// 初始化
HttpResponse *HttpResponseInit(void);
// 销毁
void httpResponseDestroy(HttpResponse *response);
// 添加响应头
void httpResponseAddHeader(HttpResponse *response, const char *key, const char *value);
// 组织http响应数据
void httpResponsePrepareMsg(HttpResponse *response, Buffer *sendBuf, int socket);
