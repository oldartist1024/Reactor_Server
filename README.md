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



## 总体结构（多 Reactor）

- **主线程 mainLoop（主 Reactor）**：只负责监听 `lfd` 的可读事件并 `accept()` 新连接。
- **子线程 evLoop（子 Reactor）**：每个子线程各自跑一个 `EventLoop`，负责已建立连接 `cfd` 的读写事件与业务处理（HTTP 解析/响应）。
- **ThreadPool**：负责创建/管理子线程，并把新连接按轮询分配到某个子 Reactor。

---

## Buffer（`Buffer.c/.h`）

**作用**：连接级别的输入/输出缓存（读缓冲、写缓冲），避免直接 `recv/send` 带来的零散读写、粘包拆包不便。

**关键点**

- 用 `readPos/writePos` 表示“已读/已写”边界。
- `bufferExtendRoom()`：不够写时先尝试“搬移可读区到头部”再决定 `realloc` 扩容。
- `bufferSocketRead()`：用 `readv()` 做“socket -> buffer + 临时大块”的聚合读（减少一次扩容/拷贝机会）。
- `bufferFindCRLF()`：用 `memmem()` 在可读区查找 `\r\n`，支撑 HTTP 行/头解析。
- `bufferSendData()`：从可读区发送并推进 `readPos`。

---

## Channel（`Channel.c/.h`）

**作用**：对“一个 fd + 关注事件 + 回调函数”的封装，是 Reactor 中的事件句柄。

**核心字段**

- `fd`：被监听的文件描述符（监听 fd 或连接 fd）。
- `events`：当前关注的事件位（`ReadEvent/WriteEvent`）。
- `readCallback/writeCallback/destroyCallback`：事件触发后的处理函数（回调参数为 `arg`）。

**典型用法**

- 监听 socket：`channelInit(lfd, ReadEvent, acceptConnection, ...)`
- 连接 socket：`channelInit(cfd, ReadEvent, processRead, processWrite, tcpConnectionDestroy, conn)`
- `writeEventEnable()`：动态打开/关闭写事件，用于“有数据要发才监听写事件”。

---

## ChannelMap（`ChannelMap.c/.h`）

**作用**：`fd -> Channel*` 的快速映射表（数组实现），用于事件激活时从 fd 找回对应的 `Channel`。

**关键点**

- 本质是 `struct Channel** list`，下标就是 fd。
- `makeMapRoom()`：当 fd 超过当前容量时按 *2 扩容，保证 `O(1)` 访问。

---

## Dispatcher（`Dispatcher.h` + `Select/Poll/EpollDispatcher.c`）

**作用**：把底层 IO 多路复用（`select/poll/epoll`）抽象成统一接口，`EventLoop` 不关心具体使用哪一种。

**接口**

- `init/add/remove/modify/dispatch/clear`

**在工程里的实际绑定**

- `eventLoopInitEx()` 中：`evLoop->dispatcher = &SelectDispatcher;`
- `dispatch()` 负责等待事件并在就绪后调用 `eventActivate()` 触发上层回调。

### epoll/poll/select 的实现差异

三者都实现 `Dispatcher` 接口（`init/add/remove/modify/dispatch/clear`），`EventLoop` 只调用接口，不关心底层机制。

- **SelectDispatcher**
  - `SelectData` 维护 `fd_set readSet/writeSet`（固定上限 `Max=1024`）。
  - `dispatch()`：`select()` 返回后遍历 `0..Max-1`，用 `FD_ISSET` 判断就绪，再 `eventActivate(evLoop, fd, ReadEvent/WriteEvent)`。
  - `remove()` / `epollRemove()` / `pollRemove()` 都会调用 `channel->destroyCallback(channel->arg)` 释放 `TcpConnection` 资源。

- **PollDispatcher**
  - `PollData` 维护 `struct pollfd fds[Max]`，并记录 `maxfd`（这里是数组下标最大值，不是 fd 数值）。
  - `dispatch()`：`poll(fds, maxfd+1, timeout)` 后遍历数组，根据 `revents` 触发 `eventActivate()`。

- **EpollDispatcher**
  - `EpollData` 维护 `epfd + events[]`（`Max=520`）。
  - `dispatch()`：`epoll_wait()` 返回就绪数量后，仅遍历就绪项；每个事件项用 `EPOLLIN/EPOLLOUT` 映射到 `ReadEvent/WriteEvent`，调用 `eventActivate()`。
  - `epollCtl()` 将 `Channel.events` 位转换成 `EPOLLIN/EPOLLOUT`，用于 add/del/mod。

> 代码里 `eventLoopInitEx()` 当前绑定的是 `SelectDispatcher`：`evLoop->dispatcher = &SelectDispatcher;`（可切换成 `&EpollDispatcher` 或 `&PollDispatcher` 来替换底层模型）。

---

## EventLoop（`EventLoop.c/.h`）

**作用**：Reactor 的核心循环与跨线程任务投递机制。

- “事件分发”：调用 `dispatcher->dispatch()`
- “事件处理”：`eventActivate()` 根据 `fd` 找到 `Channel` 并执行回调
- “任务队列”：跨线程安全地添加/删除/修改监听的 fd（`eventLoopAddTask()`）

**关键机制：跨线程唤醒**

- `socketpair()` 创建 `evLoop->socketPair[2]`
- 子线程把 `socketPair[1]` 注册成一个 `Channel(ReadEvent)`（回调 `readLocalMessage`）
- 其他线程调用 `eventLoopAddTask()` 时：
  - 若不是当前 loop 所在线程：`taskWakeup()` 往 `socketPair[0]` 写数据，唤醒被 `select/poll/epoll` 阻塞的子线程，使其及时处理任务队列。

---

## TcpServer（`TcpServer.c/.h`）

**作用**：服务器入口与主 Reactor 的组织者：创建监听 socket、主 loop、线程池，并启动服务。

**流程（对应代码）**

- `tcpServerInit(port, threadNum)`
  - `listenerInit()` 创建 `lfd` 并 `bind/listen`
  - `mainLoop = eventLoopInit()`
  - `threadPool = threadPoolInit(mainLoop, threadNum)`
- `tcpServerRun()`
  - `threadPoolRun()` 启动子线程（子 Reactor）
  - 为 `lfd` 创建 `Channel(ReadEvent)`，读回调是 `acceptConnection`
  - `eventLoopRun(mainLoop)` 启动主 Reactor

---

## TcpConnection（`TcpConnection.c/.h`）

**作用**：一个客户端连接的上下文对象，绑定：

- `cfd` 对应的 `Channel`
- `readBuf/writeBuf`
- HTTP 解析与响应对象（`HttpRequest/HttpResponse`）

**回调关系**

- `processRead()`（读事件）
  - `bufferSocketRead()` 收数据进 `readBuf`
  - `parseHttpRequest()` 解析 HTTP，并把响应内容写入 `writeBuf`
    -（可选宏 `MSG_SEND_AUTO`）打开写事件并 `MODIFY`，让写回调驱动发送
  - 解析失败则写入 `400 Bad Request`
  - 最终通常会投递 `DELETE`（示例里偏向短连接：处理完即断开）
- `processWrite()`（写事件）
  - `bufferSendData(writeBuf)`
  - 发送完：关闭写事件、`MODIFY` 更新监听集合、再 `DELETE` 关闭连接
- `tcpConnectionDestroy()`（销毁回调）
  - 调 `destroyChannel()`（从 map 移除、close fd、free channel）
  - 释放 `Buffer/HttpRequest/HttpResponse/conn`

---

## ThreadPool（`ThreadPool.c/.h`）

**作用**：管理多个 `WorkerThread`，并提供“把新连接分配给某个子 Reactor”的策略。

**关键点**

- `threadPoolRun()`：依次启动 `WorkerThread`，每个线程内部创建自己的 `EventLoop`
- `takeWorkerEventLoop()`：轮询（round-robin）选择 `workerThreads[index].evLoop`，用于 `acceptConnection()` 把 `cfd` 分发到某个子 loop

---

## WorkerThread（`WorkerThread.c/.h`）

**作用**：线程载体 + 子 Reactor 的创建者。

- `subThreadRunning()`：在线程函数里 `eventLoopInitEx(name)` 创建该线程专属 `EventLoop`，然后 `eventLoopRun()`
- 用 `mutex + cond` 保证主线程在 `workerThreadRun()` 返回前，子线程已经把 `evLoop` 初始化完成（避免主线程拿到空指针）

---

## 串起来的主流程（按调用链）

1. `TcpServerRun()`：启动线程池；主 loop 监听 `lfd`。
2. 有新连接：主 loop 的 `acceptConnection()` 被激活：
   - `accept(lfd)` 得到 `cfd`
   - `takeWorkerEventLoop()` 选一个子 loop
   - `tcpConnectionInit(cfd, subLoop)`：为连接创建 `TcpConnection` + `Channel(ReadEvent)`，并 `eventLoopAddTask(subLoop, ADD)`
3. 子 loop 收到投递任务（必要时被 `socketpair` 唤醒），把 `cfd` 加入 dispatcher 监听。
4. `cfd` 可读：触发 `processRead()`：读入 Buffer、解析 HTTP、准备响应、（可选）监听写事件、最后关闭连接。
5. 连接关闭：通过 `DELETE` 触发 dispatcher 的 `remove()`，再调用 `destroyCallback` 释放 `TcpConnection` 相关资源。

---

## main / HTTP / 连接生命周期（补充调用链）

### main 启动链

- `main()`：
  - `tcpServerInit(port, 4)` 创建 `listener + mainLoop + threadPool`
  - `tcpServerRun(server)` 启动线程池，并在主 Reactor 监听 `lfd`
- 注意：`main.c` 里硬编码了 `chdir("/home/robin/luffy");`，这是 Linux 路径（在 Windows 上运行需调整工作目录逻辑）。

### 新连接进入子 Reactor 的链路

- 主 loop 监听 `lfd` 可读 → `acceptConnection()`：
  - `accept(lfd) -> cfd`
  - `takeWorkerEventLoop(threadPool)` 轮询拿到一个子线程 `EventLoop*`
  - `tcpConnectionInit(cfd, subLoop)`：
    - 创建 `TcpConnection`（包含 `readBuf/writeBuf/request/response/channel`）
    - `channelInit(cfd, ReadEvent, processRead, processWrite, tcpConnectionDestroy, conn)`
    - `eventLoopAddTask(subLoop, channel, ADD)`（跨线程投递任务，必要时通过 `socketpair` 唤醒子 loop）

### HTTP 解析与响应链路（读事件驱动）

- `processRead()`：
  1. `bufferSocketRead(readBuf, fd)` 读入请求
  2. `parseHttpRequest(request, readBuf, response, writeBuf, socket)`：
     - 解析请求行/请求头（用 `bufferFindCRLF()` + `memmem()`）
     - `processHttpRequest()` 根据 URL 映射文件/目录，设置：
       - `response->statusCode/statusMsg/fileName`
       - `httpResponseAddHeader()`
       - `response->sendDataFunc = sendFile/sendDir`
     - `httpResponsePrepareMsg()` 写入状态行+响应头+空行，并发送 body（`sendFile/sendDir`）
  3. 该工程默认倾向“处理完即关闭连接”：`eventLoopAddTask(evLoop, channel, DELETE)`（宏 `MSG_SEND_AUTO` 打开后可走“写事件发送”路径）

### 连接释放链路（DELETE → destroyCallback）

- `eventLoopProcessTask()` 处理 `DELETE`：
  - `dispatcher->remove(channel, evLoop)`（select/poll/epoll 的 remove 中都会调用）
  - `channel->destroyCallback(channel->arg)` → `tcpConnectionDestroy()`：
    - `destroyChannel()`：从 `ChannelMap` 解绑、`close(fd)`、`free(channel)`
    - 释放 `Buffer/HttpRequest/HttpResponse/TcpConnection`





