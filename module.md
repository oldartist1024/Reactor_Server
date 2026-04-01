# 项目模块分析

本项目是一个基于 C++ 实现的多 Reactor 模式的轻量级 Web 服务器。代码结构清晰，模块化程度高，主要分为以下几个核心模块：

## 1. Reactor 核心模块

这是实现“主-从 Reactor”并发模式的基础，由事件循环、线程池和工作线程组成。

### 1.1 EventLoop (事件循环)

- **文件**: `EventLoop.h`, `EventLoop.cpp`
- **功能**:
  - **Reactor 实例**: 每个 `EventLoop` 对象都是一个独立的 Reactor 实例，负责驱动其所在线程的事件循环。
  - **事件分发**: 内部持有 `Dispatcher` 实例，通过调用其 `dispatch()` 方法来监听 I/O 事件。
  - **事件处理**: 当 `Dispatcher` 检测到就绪事件后，`EventLoop` 会根据 `fd` 从 `m_channelMap` 中找到对应的 `Channel`，并调用其注册的回调函数（`readHandler` 或 `writeHandler`）。
  - **跨线程任务处理**:
    - 维护一个线程安全的任务队列 `m_taskQ`，用于接收来自其他线程的事件注册/修改/删除请求。
    - 使用 `socketpair` 创建一对非阻塞套接字 `m_socketPair` 作为内部通信机制。当其他线程向任务队列添加任务时，会向 `m_socketPair[0]` 写入一个字节，从而唤醒阻塞在 `dispatch()` 上的 `EventLoop` 线程，使其能够及时处理任务队列中的任务。

### 1.2 ThreadPool (线程池) & WorkerThread (工作线程)

- **文件**: `ThreadPool.h`, `ThreadPool.cpp`, `WorkerThread.h`, `WorkerThread.cpp`
- **功能**:
  - **线程管理**: `ThreadPool` 负责创建、管理和启动一组 `WorkerThread`。
  - **子 Reactor 创建**: 每个 `WorkerThread` 在其独立的线程中创建一个 `EventLoop` 实例（子 Reactor），并运行其事件循环。
  - **负载均衡**: `ThreadPool` 提供 `takeWorkerEventLoop()` 方法，使用**轮询 (Round-Robin)** 算法从线程池中选择一个 `WorkerThread` 的 `EventLoop` 实例。主 Reactor 通过此方法将新接受的客户端连接均匀地分发给子 Reactor，从而实现负载均衡。
  - **线程同步**: `WorkerThread` 使用互斥锁和条件变量来确保在主线程返回之前，子线程中的 `EventLoop` 已经成功初始化，避免了主线程访问到空指针。

## 2. 事件处理与 I/O 模块

这个层次封装了底层的 I/O 操作和事件处理逻辑。

### 2.1 Dispatcher (事件分发器)

- **文件**: `Dispatcher.h`, `EpollDispatcher.cpp`, `pollDispatcher.cpp`, `selectDispatcher.cpp`
- **功能**:
  - **统一接口**: 定义了事件分发器的抽象基类 `Dispatcher`，提供了 `add`, `remove`, `modify`, `dispatch` 等统一接口。
  - **多路复用实现**: 提供了 `select`, `poll`, `epoll` 三种具体的 I/O 多路复用实现。`EventLoop` 通过持有 `Dispatcher` 的基类指针来调用这些接口，从而与底层的具体实现解耦。
  - **可扩展性**: 这种设计使得在未来可以方便地引入新的 I/O 多路复用技术（如 `kqueue`），而无需修改上层代码。

### 2.2 Channel (事件通道)

- **文件**: `Channel.h`, `Channel.cpp`
- **功能**:
  - **事件句柄**: `Channel` 是 Reactor 模式中的核心概念，它封装了一个文件描述符 (`m_fd`)、该描述符所关注的事件 (`m_events`) 以及事件发生时的回调函数（`readHandler`, `writeHandler`, `destroyCallback`）。
  - **逻辑解耦**: 将 I/O 事件与具体的业务处理逻辑（回调函数）解耦，使得事件处理流程更加清晰。
  - **动态事件管理**: 提供了 `writeEventEnable()` 方法，可以动态地启用或禁用对写事件的监听，这在需要发送大量数据时非常有用（只有当写缓冲区满时才需要监听写就绪事件）。

### 2.3 Buffer (缓冲区)

- **文件**: `Buffer.h`, `Buffer.cpp`
- **功能**:
  - **数据缓冲**: 为每个 TCP 连接提供了独立的读写缓冲区，避免了频繁的系统调用，提高了 I/O 效率。
  - **自动扩容**: 当缓冲区空间不足时，能够自动进行内存的重新分配和扩展。
  - **解决粘包/拆包**: 通过 `readPos` 和 `writePos` 指针来管理数据，可以方便地处理 TCP 协议中常见的粘包和拆包问题。
  - **HTTP 解析支持**: 提供了 `FindCRLF()` 方法，用于在缓冲区中查找 `\r\n` 分隔符，为 HTTP 请求的行解析提供了便利。

## 3. HTTP 协议处理模块

这一层负责具体的 HTTP 请求解析和响应构建。

### 3.1 HttpRequest (HTTP 请求)

- **文件**: `HttpRequest.h`, `HttpRequest.cpp`
- **功能**:
  - **状态机解析**: 使用一个简单的状态机 (`HttpRequestState`) 来解析 HTTP 请求，依次处理请求行、请求头和请求体。
  - **请求信息提取**: 从请求中提取出请求方法 (`m_method`)、URL (`m_url`) 和 HTTP 版本 (`m_version`) 等信息。
  - **URL 解码**: 提供了 `decodeMsg()` 方法来处理 URL 中的百分号编码。
  - **静态资源处理**:
    - 根据解析出的 URL，判断客户端请求的是文件还是目录。
    - 使用 `stat()` 系统调用获取文件或目录的属性。
    - 为 `HttpResponse` 模块准备必要的信息（如文件路径、文件大小等）。
    - 提供了 `sendFile` 和 `sendDir` 两个静态方法，用于实际发送文件和目录列表。

### 3.2 HttpResponse (HTTP 响应)

- **文件**: `HttpResponse.h`, `HttpResponse.cpp`
- **功能**:
  - **响应构建**: 负责构建 HTTP 响应报文，包括状态行、响应头和响应体。
  - **状态码管理**: 定义了 `HttpStatusCode` 枚举和对应的状态消息，可以方便地设置响应的状态码。
  - **响应头管理**: 提供了 `AddHeader()` 方法来添加自定义的响应头。
  - **响应发送**:
    - `PrepareMsg()` 方法负责将状态行和响应头写入 `Buffer`。
    - 通过一个函数指针 `sendDataFunc`，调用 `HttpRequest` 中对应的 `sendFile` 或 `sendDir` 方法来发送响应体。

## 4. 服务器框架与连接管理模块

这是将所有模块整合在一起，构成完整 Web 服务器的最上层。

### 4.1 TcpServer (TCP 服务器)

- **文件**: `TcpServer.h`, `TcpServer.cpp`
- **功能**:
  - **服务器启动器**: `TcpServer` 是整个服务器的入口和组织者。
  - **主 Reactor 初始化**: 在构造函数中，它会创建一个主 `EventLoop`（主 Reactor）和一个 `ThreadPool`。
  - **监听套接字设置**: 调用 `listenerInit()` 创建监听套接字 `m_lfd`，并设置端口复用。
  - **连接接受与分发**:
    - `Run()` 方法会启动线程池，并将监听套接字 `m_lfd` 的 `READ_EVENT` 注册到主 Reactor 中，其回调函数为 `acceptConnection`。
    - 当有新的客户端连接到来时，主 Reactor 调用 `acceptConnection()`，该函数会 `accept()` 新的连接，然后通过 `m_threadPool->takeWorkerEventLoop()` 将新连接的 `cfd` 分发给一个子 Reactor 进行处理。

### 4.2 TcpConnection (TCP 连接)

- **文件**: `TcpConnection.h`, `TcpConnection.cpp`
- **功能**:
  - **连接上下文**: 每个 `TcpConnection` 对象都代表一个与客户端的连接，它封装了该连接所需的所有资源，包括：
    - 用于通信的 `Channel`。
    - 读写 `Buffer`。
    - `HttpRequest` 和 `HttpResponse` 对象。
    - 所属的 `EventLoop`。
  - **回调函数实现**: 实现了 `processRead`, `processWrite`, `tcpConnectionDestroy` 等核心回调函数。
    - `processRead()`: 当连接上有数据可读时被调用，负责从 `socket` 读取数据到 `m_ReadBuffer`，然后调用 `m_request->parseHttpRequest()` 进行 HTTP 解析和响应准备。
    - `processWrite()`: 当 `socket` 可写时被调用，负责将 `m_WriteBuffer` 中的数据发送出去。
    - `tcpConnectionDestroy()`: 当连接关闭时被调用，负责释放 `TcpConnection` 所持有的所有资源。
  - **生命周期管理**: `TcpConnection` 的生命周期由其所属的 `EventLoop` 和 `Channel` 管理。当连接需要关闭时，会向 `EventLoop` 添加一个 `REMOVE` 任务，最终触发 `tcpConnectionDestroy` 回调。

### 4.3 main (主函数)

- **文件**: `main.cpp`
- **功能**:
  - **程序入口**: 程序的入口点。
  - **服务器初始化与运行**:
    - 解析命令行参数（端口号和工作目录）。
    - 使用 `chdir()` 切换服务器的工作目录。
    - 创建 `TcpServer` 实例。
    - 调用 `server->Run()` 启动服务器。
