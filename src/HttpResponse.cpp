#include "HttpResponse.h"
HttpResponse::HttpResponse()
{
    m_statusCode = HttpStatusCode::Unknown;
    m_filename = string();
    m_header.clear();
    sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::AddHeader(const string key, const string value)
{
    if (!key.empty() && !value.empty())
    {
        m_header.insert(make_pair(key, value));
    }
}

void HttpResponse::PrepareMsg(Buffer *sendBuf, int socket)
{
    // 响应行
    char temp[512];
    memset(temp, 0, sizeof(temp));
    int code = (int)m_statusCode;
    sprintf(temp, "HTTP/1.1 %d %s\r\n", code, m_info.at(code).data());
    sendBuf->AppendString(temp);
    // 响应头
    for (auto &map_value : m_header)
    {
        sprintf(temp, "%s: %s\r\n", map_value.first.data(), map_value.second.data());
        sendBuf->AppendString(temp);
    }
    // 空行
    sendBuf->AppendString("\r\n");
#ifndef MSG_SEND_AUTO
    sendBuf->SendData(socket);
#endif
    // 执行sendFile or sendDir
    sendDataFunc(m_filename, sendBuf, socket);
}
