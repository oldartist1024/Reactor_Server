#pragma once
#include "iostream"
#include "Buffer.h"
#include "string"
#include <functional>
#include <map>
using namespace std;

// 定义状态码枚举
enum class HttpStatusCode
{
    Unknown = 0,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};
// http响应结构体
class HttpResponse
{
public:
    using responseBody = void (*)(const string fileName, Buffer *sendBuf, int socket);
    // 初始化
    HttpResponse();
    // 销毁
    ~HttpResponse();
    // 添加响应头
    void AddHeader(const string key, const string value);
    // 将响应数据写入WriteBuffer
    void PrepareMsg(Buffer *sendBuf, int socket);
    inline void setFileName(string name)
    {
        m_filename = name;
    }
    inline void setStatusCode(HttpStatusCode code)
    {
        m_statusCode = code;
    }
    // responseBody m_sendDataFunc; // 发送响应体的函数指针
    responseBody sendDataFunc;

private:
    HttpStatusCode m_statusCode;  // 状态码
    string m_filename;            // 响应的文件名或者目录名
    map<string, string> m_header; // 响应头数组

    // 状态消息
    const map<int, string> m_info = {
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"},
    };
};
