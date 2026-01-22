#include "HttpRequest.h"
#define HeaderSize 12
HttpRequest *HttpRequestInit(void)
{
    HttpRequest *httprequest = (HttpRequest *)malloc(sizeof(HttpRequest));
    httpRequestReset(httprequest);
    httprequest->reqHeaders = (RequestHeader *)malloc(sizeof(RequestHeader) * HeaderSize);
    return httprequest;
}

void httpRequestReset(HttpRequest *httprequest)
{
    httprequest->method = nullptr;
    httprequest->url = nullptr;
    httprequest->version = nullptr;
    httprequest->reqHeadersNum = 0;
    httprequest->curState = ParseReqLine;
}

void httpRequestResetEX(HttpRequest *httprequest)
{
    free(httprequest->method);
    free(httprequest->url);
    free(httprequest->version);
    for (int i = 0; i < httprequest->reqHeadersNum; i++)
    {
        free(httprequest->reqHeaders[i].key);
        free(httprequest->reqHeaders[i].value);
    }
    free(httprequest->reqHeaders);
    httpRequestReset(httprequest);
}
void httpRequestDestroy(HttpRequest *httprequest)
{
    if (httprequest)
    {
        httpRequestResetEX(httprequest);
        free(httprequest);
    }
}

HttpRequestState httpRequestState(HttpRequest *httprequest)
{
    return httprequest->curState;
}

void httpRequestAddHeader(HttpRequest *httprequest, char *key, char *value)
{
    httprequest->reqHeaders[httprequest->reqHeadersNum].key = key;
    httprequest->reqHeaders[httprequest->reqHeadersNum].value = value;
    httprequest->reqHeadersNum++;
}

char *httpRequestGetHeader(HttpRequest *httprequest, const char *key)
{
    if (httprequest || key)
    {
        for (int i = 0; i < httprequest->reqHeadersNum; i++)
        {
            if (strcmp(httprequest->reqHeaders[i].key, key) == 0)
            {
                return httprequest->reqHeaders[i].value;
            }
        }
    }
    return nullptr;
}
char *splitRequestLine(const char *start, char *end, const char *sub, char **ptr)
{
    char *space = end;
    if (sub != NULL)
    {
        space = (char *)memmem(start, end - start, sub, strlen(sub));
        assert(space != NULL);
    }
    int length = space - start;
    char *tmp = (char *)malloc(length + 1);
    strncpy(tmp, start, length);
    tmp[length] = '\0';
    *ptr = tmp;
    return space + 1;
}
bool parseHttpRequestLine(HttpRequest *httprequest, Buffer *readBuf)
{
    char *end = bufferFindCRLF(readBuf);
    char *start = readBuf->data + readBuf->readPos;
    int lineLength = end - start;
    if (lineLength)
    {
        start = splitRequestLine(start, end, " ", &httprequest->method);
        start = splitRequestLine(start, end, " ", &httprequest->url);
        splitRequestLine(start, end, nullptr, &httprequest->version);
        readBuf->readPos += lineLength;
        readBuf->readPos += 2;
        httprequest->curState = ParseReqHeaders;
        return true;
    }
    return false;
}
// 该函数处理请求头中的一行
bool parseHttpRequestHeader(struct HttpRequest *request, struct Buffer *readBuf)
{
    char *end = bufferFindCRLF(readBuf);
    if (end != NULL)
    {
        char *start = readBuf->data + readBuf->readPos;
        int lineSize = end - start;
        // 基于: 搜索字符串
        char *middle = (char *)memmem(start, lineSize, ": ", 2);
        if (middle != NULL)
        {
            char *key = (char *)malloc(middle - start + 1);
            strncpy(key, start, middle - start);
            key[middle - start] = '\0';

            char *value = (char *)malloc(end - middle - 2 + 1);
            strncpy(value, middle + 2, end - middle - 2);
            value[end - middle - 2] = '\0';

            httpRequestAddHeader(request, key, value);
            // 移动读数据的位置
            readBuf->readPos += lineSize;
            readBuf->readPos += 2;
        }
        else
        {
            // 请求头被解析完了, 跳过空行
            readBuf->readPos += 2;
            // 修改解析状态
            // 忽略 post 请求, 按照 get 请求处理
            request->curState = ParseReqDone;
        }
        return true;
    }
    return false;
}

bool parseHttpRequest(HttpRequest *httprequest, Buffer *readBuf, HttpResponse *response, Buffer *sendbuf, int socket)
{
    bool flag = true;
    while (httprequest->curState != ParseReqDone)
    {
        switch (httprequest->curState)
        {
        case ParseReqLine:
            flag = parseHttpRequestLine(httprequest, readBuf);
            break;
        case ParseReqHeaders:
            flag = parseHttpRequestHeader(httprequest, readBuf);
            break;
        case ParseReqBody:
            break;
        default:
            break;
        }
        if (!flag)
            return flag;
    }
    if (httprequest->curState == ParseReqDone)
    {
        // 处理请求
        processHttpRequest(httprequest, response);
        // 发送给客户端
        httpResponsePrepareMsg(response, sendbuf, socket);
    }
    httprequest->curState = ParseReqLine;
    return flag;
}

// 处理基于get的http请求
bool processHttpRequest(HttpRequest *httprequest, HttpResponse *response)
{
    if (strcasecmp(httprequest->method, "get") != 0)
    {
        return false;
    }
    decodeMsg(httprequest->url, httprequest->url);
    // 处理客户端请求的静态资源(目录或者文件)
    char *file = NULL;
    if (strcmp(httprequest->url, "/") == 0)
    {
        file = "./";
    }
    else
    {
        file = httprequest->url + 1;
    }
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在 -- 回复404
        // sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        // sendFile("404.html", cfd);
        // 响应行
        response->statusCode = NotFound;
        strcpy(response->statusMessage, "Not Found");
        // 响应头
        httpResponseAddHeader(response, "Content-Type", getFileType(".html"));
        // 响应体
        strcpy(response->filename, "404.html");
        response->sendDataFunc = sendFile;
        return false;
    }
    // 判断文件类型
    if (S_ISDIR(st.st_mode))
    {
        // 把这个目录中的内容发送给客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        // sendDir(file, cfd);
        // 响应行
        response->statusCode = OK;
        strcpy(response->statusMessage, "OK");
        // 响应头
        httpResponseAddHeader(response, "Content-Type", getFileType(".html"));
        // 响应体
        strcpy(response->filename, file);
        response->sendDataFunc = sendDir;
    }
    else
    {
        // 把文件的内容发送给客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        // sendFile(file, cfd);
        // 响应行
        response->statusCode = OK;
        strcpy(response->statusMessage, "OK");
        // 响应头
        char temp[32];
        sprintf(temp, "%ld", st.st_size);
        httpResponseAddHeader(response, "Content-Type", getFileType(file));
        httpResponseAddHeader(response, "Content-length", temp);
        // 响应体
        strcpy(response->filename, file);
        response->sendDataFunc = sendFile;
    }
    return false;
}
// 将字符转换为整形数
int hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char *to, char *from)
{
    for (; *from != '\0'; ++to, ++from)
    {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝, 赋值
            *to = *from;
        }
    }
    *to = '\0';
}
const char *getFileType(const char *name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char *dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8"; // 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
void sendDir(const char *dirName, struct Buffer *sendBuf, int cfd)
{
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
    struct dirent **namelist;
    int num = scandir(dirName, &namelist, NULL, alphasort);
    for (int i = 0; i < num; ++i)
    {
        // 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
        char *name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dirName, name);
        stat(subPath, &st);
        if (S_ISDIR(st.st_mode))
        {
            // a标签 <a href="">name</a>
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }
        else
        {
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }
        // send(cfd, buf, strlen(buf), 0);
        bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
        bufferSendData(sendBuf, cfd);
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    // send(cfd, buf, strlen(buf), 0);
    bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, cfd);
#endif
    free(namelist);
}

void sendFile(const char *fileName, struct Buffer *sendBuf, int cfd)
{
    // 1. 打开文件
    int fd = open(fileName, O_RDONLY);
    assert(fd > 0);
#if 1
    while (1)
    {
        char buf[1024];
        int len = read(fd, buf, sizeof buf);
        if (len > 0)
        {
            // send(cfd, buf, len, 0);
            bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
            bufferSendData(sendBuf, cfd);
#endif
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            close(fd);
            perror("read");
        }
    }
#else
    off_t offset = 0;
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while (offset < size)
    {
        int ret = sendfile(cfd, fd, &offset, size - offset);
        printf("ret value: %d\n", ret);
        if (ret == -1 && errno == EAGAIN)
        {
            printf("没数据...\n");
        }
    }
#endif
    close(fd);
}
