#include "HttpRequest.h"
HttpRequest::HttpRequest()
{
    Reset();
}

HttpRequest::~HttpRequest()
{
}
void HttpRequest::Reset()
{
    m_method = m_url = m_version = string();
    m_reqHeaders.clear();                        // 请求头数组
    m_curState = HttpRequestState::ParseReqLine; // 当前解析状态
}

void HttpRequest::AddHeader(const string key, const string value)
{
    if (!key.empty() && !value.empty())
    {
        m_reqHeaders.insert(make_pair(key, value));
    }
}

bool HttpRequest::parseHttpRequestLine(Buffer *readBuf)
{
    // 找到第一个\r\n
    char *end = readBuf->FindCRLF();
    if (end == nullptr)
    {
        return false;
    }
    // 指向开头
    char *start = readBuf->data_position();
    // 计算请求行的长度
    int lineLength = end - start;
    if (lineLength)
    {
        // 拆分请求行
        start = splitRequestLine(start, end, " ", m_method);
        start = splitRequestLine(start, end, " ", m_url);
        splitRequestLine(start, end, nullptr, m_version);
        // 跳过字符串
        readBuf->readPosIncrease(lineLength);
        // 跳过\r\n
        readBuf->readPosIncrease(2);
        m_curState = HttpRequestState::ParseReqHeaders;
        return true;
    }
    return false;
}
// 该函数处理请求头中的一行（当请求头有多行的时候，需要循环处理）
bool HttpRequest::parseHttpRequestHeader(Buffer *readBuf)
{
    char *end = readBuf->FindCRLF();
    if (end != NULL)
    {
        char *start = readBuf->data_position();
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

            AddHeader(key, value);
            // 移动读数据的位置
            readBuf->readPosIncrease(lineSize);
            readBuf->readPosIncrease(2);
        }
        else
        {
            // 请求头被解析完了, 跳过空行
            readBuf->readPosIncrease(2);
            // 修改解析状态
            // 忽略 post 请求, 按照 get 请求处理
            m_curState = HttpRequestState::ParseReqDone;
        }
        return true;
    }
    return false;
}
bool HttpRequest::parseHttpRequest(Buffer *readBuf, HttpResponse *response, Buffer *sendbuf, int socket)
{
    bool flag = true;
    while (m_curState != HttpRequestState::ParseReqDone)
    {
        // 状态机
        switch (m_curState)
        {
        case HttpRequestState::ParseReqLine: // 请求行
            flag = parseHttpRequestLine(readBuf);
            break;
        case HttpRequestState::ParseReqHeaders: // 请求头（循环处理）
            flag = parseHttpRequestHeader(readBuf);
            break;
        case HttpRequestState::ParseReqBody: // 请求体
            break;
        default:
            break;
        }
        if (!flag) // 如果解析失败了，直接返回
            return flag;
    }
    // 解析完毕
    if (m_curState == HttpRequestState::ParseReqDone)
    {
        // 处理http请求
        processHttpRequest(response);
        // 将响应数据写入WriteBuffer
        response->PrepareMsg(sendbuf, socket);
    }
    // 重置状态机
    m_curState = HttpRequestState::ParseReqLine;
    return flag;
}
// 处理基于get的http请求，并且给响应的response结构体赋值
bool HttpRequest::processHttpRequest(HttpResponse *response)
{
    // 判断是不是get请求
    if (strcasecmp(m_method.c_str(), "get") != 0)
    {
        return false;
    }
    // 解码
    m_url = decodeMsg(m_url);
    // 处理客户端请求的静态资源(目录或者文件)
    char *file = NULL;
    if (strcmp(m_url.c_str(), "/") == 0)
    {
        file = const_cast<char *>("./");
    }
    else
    {
        file = const_cast<char *>(m_url.c_str()) + 1;
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
        response->setStatusCode(HttpStatusCode::NotFound);
        // 响应头
        response->AddHeader("Content-Type", getFileType(".html"));
        // 响应体
        response->setFileName("404.html");
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
        response->setStatusCode(HttpStatusCode::OK);
        // 响应头
        response->AddHeader("Content-Type", getFileType(".html"));
        // 响应体
        response->setFileName(file);
        response->sendDataFunc = sendDir;
    }
    else
    {
        // 把文件的内容发送给客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        // sendFile(file, cfd);
        // 响应行
        response->setStatusCode(HttpStatusCode::OK);
        // 响应头
        char temp[32];
        sprintf(temp, "%ld", st.st_size);
        response->AddHeader("Content-Type", getFileType(file));
        response->AddHeader("Content-length", temp);
        // 响应体
        response->setFileName(file);
        response->sendDataFunc = sendFile;
    }
    return false;
}
const char *HttpRequest::getFileType(const char *name)
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
void HttpRequest::sendDir(string dirName, Buffer *sendBuf, int cfd)
{
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName.c_str());
    struct dirent **namelist;
    int num = scandir(dirName.c_str(), &namelist, NULL, alphasort);
    for (int i = 0; i < num; ++i)
    {
        // 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
        char *name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dirName.c_str(), name);
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
        sendBuf->AppendString(buf);
#ifndef MSG_SEND_AUTO
        sendBuf->SendData(cfd);
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    // send(cfd, buf, strlen(buf), 0);
    sendBuf->AppendString(buf);
#ifndef MSG_SEND_AUTO
    sendBuf->SendData(cfd);
#endif
    free(namelist);
}
void HttpRequest::sendFile(string fileName, Buffer *sendBuf, int cfd)
{
    // 1. 打开文件
    int fd = open(fileName.c_str(), O_RDONLY);
    assert(fd > 0);
#if 1
    while (1)
    {
        char buf[1024];
        int len = read(fd, buf, sizeof buf);
        if (len > 0)
        {
            // send(cfd, buf, len, 0);
            sendBuf->AppendString(buf, len);
#ifndef MSG_SEND_AUTO
            sendBuf->SendData(cfd);
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
// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
string HttpRequest::decodeMsg(string msg)
{
    string str = string();
    const char *from = msg.data();
    for (; *from != '\0'; ++from)
    {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝, 赋值
            str.append(1, *from);
        }
    }
    str.append(1, '\0');
    return str;
}

int HttpRequest::hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}
// 在start到end范围内, 按照sub拆分字符串, 把拆分出来的字符串赋值给ptr
// 返回找到的sub+1的位置
char *HttpRequest::splitRequestLine(const char *start, char *end, const char *sub, string &ptr)
{
    char *space = end;
    if (sub != NULL)
    {
        space = (char *)memmem(start, end - start, sub, strlen(sub));
        assert(space != NULL);
    }

    int length = space - start;
    ptr.assign(start, length); // 使用assign方法
    return space + 1;
}
