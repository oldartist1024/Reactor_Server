#include "HttpResponse.h"

HttpResponse *HttpResponseInit(void)
{
    HttpResponse *httpresponse = (HttpResponse *)malloc(sizeof(HttpResponse));
    httpresponse->statusCode = Unknown;
    httpresponse->headerCount = 0;
    httpresponse->sendDataFunc = nullptr;
    memset(httpresponse->statusMessage, 0, sizeof(httpresponse->statusMessage));
    memset(httpresponse->filename, 0, sizeof(httpresponse->filename));

    int size = ResHeaderSize * sizeof(ResponseHeader);
    httpresponse->header = (ResponseHeader *)malloc(size);
    memset(httpresponse->header, 0, size);
    return httpresponse;
}

void httpResponseDestroy(HttpResponse *response)
{
    if (response)
    {
        free(response->header);
        free(response);
    }
}

void httpResponseAddHeader(HttpResponse *response, const char *key, const char *value)
{
    if (response && key && value)
    {
        strcpy(response->header[response->headerCount].key, key);
        strcpy(response->header[response->headerCount].value, value);
        response->headerCount++;
    }
}

void httpResponsePrepareMsg(HttpResponse *response, Buffer *sendBuf, int socket)
{
    // 响应行
    char temp[512];
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMessage);
    bufferAppendString(sendBuf, temp);
    // 响应头
    for (int i = 0; i < response->headerCount; i++)
    {
        sprintf(temp, "%s: %s\r\n", response->header[i].key, response->header[i].value);
        bufferAppendString(sendBuf, temp);
    }
    // 空行
    bufferAppendString(sendBuf, "\r\n");
    // 响应内容
    response->sendDataFunc(response->filename, sendBuf, socket);
}
