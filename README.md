# 项目笔记
## 1) 主线程运行流程（Main Reactor）
1. **启动入口**：`main()`  
   - `tcpServerInit(port, threadNum)`：
     - `listenerInit()`：创建 `lfd`，`bind/listen`
     - `mainLoop = eventLoopInit()`：创建主 `EventLoop`
     - `threadPool = threadPoolInit(mainLoop, threadNum)`
2. **运行服务器**：`tcpServerRun(server)`
   - `threadPoolRun()`：启动所有子线程（子 Reactor）
   - 为 `lfd` 创建 `Channel(ReadEvent)`，回调 `acceptConnection`
   - `eventLoopAddTask(mainLoop, lfdChannel, ADD)`：把监听 fd 加入主 loop
   - `eventLoopRun(mainLoop)`：进入循环
3. **主 loop 核心工作**（循环里）
   - `dispatcher->dispatch()` 等待 `lfd` 可读
   - `eventActivate()` → 触发 `acceptConnection()`
4. **acceptConnection 回调做的事**
   - `accept(lfd) -> cfd`
   - `takeWorkerEventLoop(threadPool)` 轮询拿一个子线程 `EventLoop*`
   - `tcpConnectionInit(cfd, subLoop)`：把新连接交给子 loop

> 主线程主要职责：**接入（accept）+ 分发连接到子 Reactor**，不直接处理 HTTP 读写。

## 2）子线程运行流程

子线程在这个工程里本质上是：**每个子线程跑一个独立的 `EventLoop`（子 Reactor）**，负责它被分配到的若干连接 `cfd` 的 IO 事件与回调执行。工作过程可以按“创建阶段 → 运行阶段 → 接收任务/处理连接”理解。

---

### 1) 创建阶段：主线程启动子线程

主线程 `threadPoolRun()` 里循环：

- `workerThreadInit(&pool->workerThreads[i], i)`
- `workerThreadRun(&pool->workerThreads[i])`

`workerThreadRun()` 做两件事：

1. `pthread_create(..., subThreadRunning, thread)` 创建线程
2. 主线程用 `cond_wait` 阻塞，直到子线程把 `thread->evLoop` 创建好（避免主线程拿到空的 `evLoop`）

---

### 2) 子线程入口：subThreadRunning()

子线程函数 `subThreadRunning()`：

1. `thread->evLoop = eventLoopInitEx(thread->name)`
   - 这一步创建该线程专属的 `EventLoop`
   - 同时建立 `socketpair`，并把 `socketPair[1]` 注册为一个读事件 Channel（用于被主线程唤醒）
2. `pthread_cond_signal(&thread->cond)` 通知主线程：evLoop 就绪
3. `eventLoopRun(thread->evLoop)` 进入事件循环（一直跑）

---

### 3) 运行阶段：子线程 EventLoop 在做什么

`eventLoopRun()` 的 while 循环里主要两件事：

1. `dispatcher->dispatch(evLoop, timeout)`阻塞等待 IO 就绪（select/poll/epoll）
2. `eventLoopProcessTask(evLoop)`处理任务队列里积累的 `ADD/DELETE/MODIFY`，把这些变更真正落到：
   - `ChannelMap`（fd→channel）
   - `Dispatcher` 监听集合

---

### 4) 子线程什么时候会“接到新连接”？

新连接的 `cfd` 是主线程 `accept()` 出来的，然后主线程做：

- `evLoop = takeWorkerEventLoop(pool)`（轮询选一个子线程的 loop）
- `tcpConnectionInit(cfd, evLoop)`（仍在主线程里执行）
- `tcpConnectionInit()` 内部调用：`eventLoopAddTask(evLoop, channel, ADD)`

因为这是跨线程投递，`eventLoopAddTask()` 会：

- 把任务入队
- `taskWakeup(evLoop)` 往 `socketpair[0]` 写数据，**唤醒子线程**（让子线程从 select/poll/epoll 阻塞中返回）

子线程被唤醒后，下一轮循环会执行 `eventLoopProcessTask()`，从而把 `cfd` 加入自己的监听集合，之后这个连接的读写事件都由该子线程处理。

### 5)子线程如何处理客户端读写

当 `cfd` 可读/可写：

- `dispatcher->dispatch()` 检测到就绪
- 调 `eventActivate(evLoop, fd, event)`
- 从 `ChannelMap` 找到 channel，调用：
  - `processRead()`：收包→解析 HTTP→准备响应→通常投递 `DELETE`
  - `processWrite()`：发送 writeBuf→发送完投递 `DELETE`
  - `DELETE` 最终触发 `destroyCallback`，释放连接资源

---

总结：**子线程就是一个“独立的 Reactor 执行体”**：被唤醒/被事件驱动，更新监听集合，执行连接回调，关闭并回收连接。
## 3) 主线程与子线程之间的交互（核心：任务队列 + 唤醒机制）
交互目标：主线程把“新连接 cfd 的监听/回调”安全、及时地交给子线程处理。

### 3.1 主线程 → 子线程（投递新连接）
1. 主线程 `acceptConnection()` 里调用 `tcpConnectionInit(cfd, subLoop)`
2. `tcpConnectionInit()` 创建 `TcpConnection + Channel(cfd)`，然后：
   - `eventLoopAddTask(subLoop, cfdChannel, ADD)`  **把任务入队到子 loop 的任务队列**
3. 由于调用者是主线程（`pthread_self() != subLoop->threadID`），`eventLoopAddTask()` 会：
   - `taskWakeup(subLoop)`：往 `subLoop->socketPair[0]` 写数据
4. 子线程在 `dispatch()` 中监听了 `socketPair[1]`，因此会立即被唤醒
5. 子线程醒来后执行 `eventLoopProcessTask()`：
   - 真正 `dispatcher->add(cfdChannel)`
   - `ChannelMap[fd] = cfdChannel`

### 3.2 子线程内部自驱（修改/关闭连接）
当子线程在 `processRead/processWrite` 里需要：
- 开/关写事件 → `eventLoopAddTask(subLoop, cfdChannel, MODIFY)`
- 关闭连接 → `eventLoopAddTask(subLoop, cfdChannel, DELETE)`
因为调用者就是子线程本身，所以 `eventLoopAddTask()` 会直接在本线程处理队列（不会走唤醒）。

### 3.3 为什么必须有“唤醒 fd”？
子线程大部分时间阻塞在 `select/poll/epoll_wait`，如果不唤醒：
- 主线程投递的 ADD 任务要等到 `dispatch()` 超时（这里 2s）才会被处理
- 新连接会“已 accept 但迟迟没加入监听集合”，表现为响应延迟

总结：  
- **任务队列**解决“跨线程安全变更监听集合”  
- **socketpair 唤醒 channel**解决“及时让子线程从 dispatch 阻塞中返回并处理任务”
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




