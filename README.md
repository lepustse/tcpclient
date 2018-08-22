# tcpclient

## 1、介绍
这是基于 RT-Thread 基于多线程的非阻塞 socket 编程示例，本文主要是介绍如何使用 `tcpclient.c` API。详情可了解：[多线程的非阻塞 socket 编程](https://github.com/neverxie/tcpclient/blob/master/an0019-rtthread-tcpclient-socket/an0019-rtthread-system-tcpclient-socket.md)
### 1.1、目录结构

| 名称 | 说明 |
| ---- | ---- |
| inc  | 头文件目录 |
| src  | 源代码目录 |
| examples | 例程目录 |

### 1.2、许可证

tcpclient 遵循 LGPLv2.1 许可，详见 `LICENSE` 文件。

### 1.3、依赖
- 依赖 [EasyLogger](https://github.com/armink/EasyLogger) 软件包
- 依赖 [dfs](https://www.rt-thread.org/document/site/rtthread-development-guide/rtthread-manual-doc/zh/1chapters/12-chapter_filesystem/) 文件系统
- RT-Thread 3.0+，对 bsp 无依赖

## 2、使用 tcpclient

测试 `tcpclient.c` 可参考例程代码，该代码位于： [tcpclient_example.c](https://github.com/neverxie/tcpclient/blob/master/examples/tcpclient_example.c)


## 3、 API 介绍

### 3.1、启动 TCP 客户端任务
`rt_tcpclient_t *rt_tcpclient_start(const char *hostname, rt_uint32_t port);`

| 参数 | 描述 |
| ---- | ---- |
| hostname  | IP 地址或域名 |
| port | 端口号 |
|返回|描述|
|tcpclient 对象指针|创建 TCP 客户端任务成功|
|RT_NULL|创建 TCP 客户端任务失败 |


>输入服务器 IP 地址 & （自定义的）端口号；服务器（网络调试助手）监听这个端口号

### 3.2、关闭 TCP 客户端任务
`void rt_tcpclient_close(rt_tcpclient_t *thiz);`

| 参数 | 描述 |
| ---- | ---- |
| thiz  | tcp client 对象 |
|返回|描述|
| 无  | 无 |

>通信结束，用户使用此 API 关闭资源

### 3.3、注册接收数据的回调函数
`void rt_tcpclient_attach_rx_cb(rt_tcpclient_t *thiz, rx_cb_t cb);`

| 参数 | 描述 |
| ---- | ---- |
| thiz  | tcp client 对象 |
| cb  | 回调函数指针 |
|返回|描述|
| 无  | 无 |

>回调函数需要由用户来写，可参考例程代码

### 3.4、发送数据
`rt_size_t rt_tcpclient_send(rt_tcpclient_t *thiz, const void *buff, rt_size_t len);`

| 参数 | 描述 |
| ---- | ---- |
| thiz  | tcp client 对象 |
| buff  | 要发送的数据 |
| len | 数据长度 |
|返回|描述|
| > 0  | 成功，返回发送的数据的长度 |
|<= 0|失败|


## 4、注意事项

通信完毕，需要用户自己调用 `rt_tcpclient_close()` API 释放资源

## 5、联系方式 & 感谢

* 感谢：[armink](https://github.com/armink/EasyLogger) 制作了 EasyLogger 软件包
* 维护：[never](https://github.com/neverxie)
