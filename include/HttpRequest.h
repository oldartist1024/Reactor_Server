#pragma once
#include <iostream>
#include "Buffer.h"
#include <cassert>
#include <sys/stat.h>
#include "HttpResponse.h"
#include "unistd.h"
#include <fcntl.h>
#include "Buffer.h"
#include <dirent.h>
#include "TcpConnection.h"
// 请求头结构
struct RequestHeader
{
    char *key;   // 键
    char *value; // 值
};
// 当前的解析状态
enum HttpRequestState
{
    ParseReqLine,    // 解析请求行
    ParseReqHeaders, // 解析请求头
    ParseReqBody,    // 解析请求体
    ParseReqDone     // 解析完成
};
// http请求结构体
struct HttpRequest
{
    // 请求行
    char *method;
    char *url;
    char *version;
    // 请求头
    struct RequestHeader *reqHeaders; // 请求头数组
    int reqHeadersNum;                // 请求头数量
    // 当前解析状态
    enum HttpRequestState curState;
};
// 初始化
HttpRequest *HttpRequestInit(void);
// 重置
void httpRequestReset(HttpRequest *httprequest);
void httpRequestResetEX(HttpRequest *httprequest);
void httpRequestDestroy(struct HttpRequest *req);
// 获取处理状态
enum HttpRequestState httpRequestState(HttpRequest *httprequest);
// 添加请求头
void httpRequestAddHeader(HttpRequest *httprequest, const char *key, const char *value);
// 查询key对应的value
char *httpRequestGetHeader(HttpRequest *httprequest, const char *key);
// 处理请求行
bool parseHttpRequestLine(HttpRequest *httprequest, Buffer *readBuf);
// 处理请求头
bool parseHttpRequestHeader(struct HttpRequest *request, struct Buffer *readBuf);
// 解析HTTP请求
bool parseHttpRequest(HttpRequest *httprequest, Buffer *readBuf, HttpResponse *response, Buffer *sendbuf, int socket);
// 处理http请求
bool processHttpRequest(HttpRequest *httprequest, HttpResponse *response);
// 解码
void decodeMsg(char *to, char *from);
// 获取文件类型
const char *getFileType(const char *name);
// 发送文件夹
void sendDir(const char *dirName, struct Buffer *sendBuf, int cfd);
// 发送文件
void sendFile(const char *fileName, struct Buffer *sendBuf, int cfd);
