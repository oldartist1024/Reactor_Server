## http笔记
#### 当浏览器打开http://192.168.232.128:10000/，浏览器作为客户端，与服务器建立连接，然后发送：

```http
GET / HTTP/1.1
Host: 192.168.232.128:10000
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36 Edg/143.0.0.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
```

#### 通过把它第一行截断后，获取的结果为：

```http
GET / HTTP/1.1
```