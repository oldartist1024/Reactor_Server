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
#include <map>
using namespace std;
// 请求头结构
struct RequestHeader
{
    char *key;   // 键
    char *value; // 值
};
// 当前的解析状态
enum class HttpRequestState
{
    ParseReqLine,    // 解析请求行
    ParseReqHeaders, // 解析请求头
    ParseReqBody,    // 解析请求体
    ParseReqDone     // 解析完成
};
// http请求结构体
class HttpRequest
{
public:
    // 初始化
    HttpRequest();
    ~HttpRequest();
    // 重置
    void Reset();
    // 添加请求头
    void AddHeader(const string key, const string value);
    // 处理请求行
    bool parseHttpRequestLine(Buffer *readBuf);
    // 处理请求头
    bool parseHttpRequestHeader(Buffer *readBuf);
    // 解析HTTP请求
    bool parseHttpRequest(Buffer *readBuf, HttpResponse *response, Buffer *sendbuf, int socket);
    // 处理http请求
    bool processHttpRequest(HttpResponse *response);
    // 获取文件类型
    const char *getFileType(const char *name);
    // 发送文件夹
    static void sendDir(string dirName, Buffer *sendBuf, int cfd);
    // 发送文件
    static void sendFile(string fileName, Buffer *sendBuf, int cfd);
    // 获取处理状态
    inline HttpRequestState GetState()
    {
        return m_curState;
    }

private:
    string decodeMsg(string from); // 解码
    int hexToDec(char c);          // 将字符转换为整形数
    char *splitRequestLine(const char *start, char *end, const char *sub, string &ptr);

private:
    // 请求行
    string m_method;
    string m_url;
    string m_version;
    // 请求头
    map<string, string> m_reqHeaders; // 请求头数组
    // 当前解析状态
    HttpRequestState m_curState;
};
