/*
 * File      : tcpclient.c
 * This file is part of RT-Thread
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-10     never        the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "elog.h"
#include "tcpclient.h"

struct rt_tcpclient
{
    int sock_fd;
    int pipe_read_fd;
    int pipe_write_fd;
    char pipe_name[8];
    rx_cb_t rx;
};

#define BUFF_SIZE (1024)
#define MAX_VAL(A, B) ((A) > (B) ? (A) : (B))
#define STRCMP(a, R, b) (strcmp((a), (b)) R 0)
#define RX_CB_HANDLE(_buff, _len)  \
    do                             \
    {                              \
        if (thiz->rx)              \
            thiz->rx(_buff, _len); \
    } while (0)

#define PRINTF_TEST(_buff, _len, _tag) \
    do                                 \
    {                                  \
        _buff[_len] = '\0';            \
        elog_d(_tag, ":%s\n", _buff);  \
    } while (0)

#define EXCEPTION_HANDLE(_bytes, _tag, _info_a, _info_b)                  \
    do                                                                    \
    {                                                                     \
        if (_bytes < 0)                                                   \
        {                                                                 \
            elog_e(_tag, _info_a, "return: %d\n", _bytes);                \
            goto exit;                                                    \
        }                                                                 \
        if (_bytes == 0)                                                  \
        {                                                                 \
            if (STRCMP(_info_b, ==, "warning"))                           \
                elog_w(_tag, _info_b, "return: %d\n", _bytes);            \
            else                                                          \
            {                                                             \
                elog_e(_tag, _info_b, "return: %d\n", _bytes);            \
                RX_CB_HANDLE("TCP disconnect", strlen("TCP disconnect")); \
                goto exit;                                                \
            }                                                             \
        }                                                                 \
    } while (0)

#define EXIT_HANDLE(_buff)                                            \
    do                                                                \
    {                                                                 \
        if (STRCMP(_buff, ==, "exit"))                                \
        {                                                             \
            elog_i("exit handle", "receive [exit], exit thread\n");   \
            elog_i("exit handle", "user clean up resources pleas\n"); \
            goto exit;                                                \
        }                                                             \
    } while (0)

static rt_tcpclient_t *tcpclient_create(void);
static rt_int32_t tcpclient_destory(rt_tcpclient_t *thiz);
static rt_int32_t socket_init(rt_tcpclient_t *thiz, const char *hostname, rt_uint32_t port);
static rt_int32_t socket_deinit(rt_tcpclient_t *thiz);
static rt_int32_t pipe_init(rt_tcpclient_t *thiz);
static rt_int32_t pipe_deinit(rt_tcpclient_t *thiz);
static void select_handle(rt_tcpclient_t *thiz, char *pipe_buff, char *sock_buff);
static rt_int32_t tcpclient_thread_init(rt_tcpclient_t *thiz);
static void tcpclient_thread_entry(void *param);

static rt_tcpclient_t *tcpclient_create(void)
{
    rt_tcpclient_t *thiz = RT_NULL;

    thiz = rt_malloc(sizeof(rt_tcpclient_t));
    if (thiz == RT_NULL)
    {
        elog_e("tcpclient alloc", "malloc error\n");
        return RT_NULL;
    }

    thiz->sock_fd = -1;
    thiz->pipe_read_fd = -1;
    thiz->pipe_write_fd = -1;
    memset(thiz->pipe_name, 0, sizeof(thiz->pipe_name));
    thiz->rx = RT_NULL;
    
    return thiz;
}

static rt_int32_t tcpclient_destory(rt_tcpclient_t *thiz)
{
    int res = 0;

    if (thiz == RT_NULL)
    {
        elog_e("tcpclient del", "param is NULL, delete failed\n");
        return -1;
    }

    if (thiz->sock_fd != -1)
        socket_deinit(thiz);

    if (thiz->pipe_read_fd != -1)
    {
        res = close(thiz->pipe_read_fd);
        RT_ASSERT(res == 0);
        thiz->pipe_read_fd = -1;
    }

    if (thiz->pipe_write_fd != -1)
    {
        res = close(thiz->pipe_write_fd);
        RT_ASSERT(res == 0);
        thiz->pipe_write_fd = -1;
    }

    free(thiz);
    thiz = RT_NULL;

    elog_i("tcpclient del", "delete succeed\n");

    return 0;
}

static rt_int32_t socket_init(rt_tcpclient_t *thiz, const char *url, rt_uint32_t port)
{
    struct sockaddr_in dst_addr;
    struct hostent *hostname;
    rt_int32_t res = 0;
    const char *str = "socket create succeed";

    if (thiz == RT_NULL)
        return -1;

    thiz->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (thiz->sock_fd == -1)
    {
        elog_e("socket init", "socket create failed\n");
        return -1;
    }

    hostname = gethostbyname(url);

    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(port);
    dst_addr.sin_addr = *((struct in_addr *)hostname->h_addr);
    memset(&(dst_addr.sin_zero), 0, sizeof(dst_addr.sin_zero));

    res = connect(thiz->sock_fd, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr));
    if (res == -1)
    {
        elog_e("socket init", "socket connect failed\n");
        return -1;
    }

    elog_i("socket init", "TCP connected succeed\n");

    send(thiz->sock_fd, str, strlen(str), 0);

    return 0;
}

static rt_int32_t socket_deinit(rt_tcpclient_t *thiz)
{
    int res = 0;

    if (thiz == RT_NULL)
    {
        elog_e("socket deinit", "param is NULL, socket deinit failed\n");
        return -1;
    }

    res = closesocket(thiz->sock_fd);
    RT_ASSERT(res == 0);

    thiz->sock_fd = -1;

    elog_i("socket deinit", "socket close succeed\n");

    return 0;
}

static rt_int32_t pipe_init(rt_tcpclient_t *thiz)
{
    char dev_name[32];
    static int pipeno = 0;
    rt_pipe_t *pipe = RT_NULL;

    if (thiz == RT_NULL)
    {
        elog_e("pipe init", "param is NULL\n");
        return -1;
    }

    snprintf(thiz->pipe_name, sizeof(thiz->pipe_name), "pipe%d", pipeno++);

    pipe = rt_pipe_create(thiz->pipe_name, PIPE_BUFSZ);
    if (pipe == RT_NULL)
    {
        elog_e("pipe create", "pipe create failed\n");
        return -1;
    }

    snprintf(dev_name, sizeof(dev_name), "/dev/%s", thiz->pipe_name);
    thiz->pipe_read_fd = open(dev_name, O_RDONLY, 0);
    if (thiz->pipe_read_fd < 0)
        goto fail_read;

    thiz->pipe_write_fd = open(dev_name, O_WRONLY, 0);
    if (thiz->pipe_write_fd < 0)
        goto fail_write;

    elog_i("pipe init", "pipe init succeed\n");
    return 0;

fail_write:
    close(thiz->pipe_read_fd);
fail_read:
    return -1;
}

static rt_int32_t pipe_deinit(rt_tcpclient_t *thiz)
{
    int res = 0;

    if (thiz == RT_NULL)
    {
        elog_e("pipe deinit", "param is NULL, pipe deinit failed\n");
        return -1;
    }

    res = close(thiz->pipe_read_fd);
    RT_ASSERT(res == 0);
    thiz->pipe_read_fd = -1;

    res = close(thiz->pipe_write_fd);
    RT_ASSERT(res == 0);
    thiz->pipe_write_fd = -1;

    rt_pipe_delete(thiz->pipe_name);

    elog_i("pipe deinit", "pipe close succeed\n");
    return 0;
}

static rt_int32_t tcpclient_thread_init(rt_tcpclient_t *thiz)
{
    rt_thread_t tcpclient_tid = RT_NULL;

    tcpclient_tid = rt_thread_create("tcpc", tcpclient_thread_entry, thiz, 2048, 12, 10);
    if (tcpclient_tid == RT_NULL)
    {
        elog_e("tcpclient thread", "thread create failed\n");
        return -1;
    }

    rt_thread_startup(tcpclient_tid);

    elog_i("tcpclient thread", "thread init succeed\n");
    return 0;
}

static void select_handle(rt_tcpclient_t *thiz, char *pipe_buff, char *sock_buff)
{
    fd_set fds;
    rt_int32_t max_fd = 0, res = 0;

    max_fd = MAX_VAL(thiz->sock_fd, thiz->pipe_read_fd) + 1;
    FD_ZERO(&fds);

    while (1)
    {
        FD_SET(thiz->sock_fd, &fds);
        FD_SET(thiz->pipe_read_fd, &fds);

        res = select(max_fd, &fds, RT_NULL, RT_NULL, RT_NULL);

        /* exception handling: exit */
        EXCEPTION_HANDLE(res, "select handle", "error", "timeout");

        /* socket is ready */
        if (FD_ISSET(thiz->sock_fd, &fds))
        {
            res = recv(thiz->sock_fd, sock_buff, BUFF_SIZE, 0);

            /* exception handling: exit */
            EXCEPTION_HANDLE(res, "socket recv handle", "error", "TCP disconnected");

            /* have received data, clear the end */
            sock_buff[res] = '\0';

            RX_CB_HANDLE(sock_buff, res);

            EXIT_HANDLE(sock_buff);
        }

        /* pipe is read */
        if (FD_ISSET(thiz->pipe_read_fd, &fds))
        {
            /* read pipe */
            res = read(thiz->pipe_read_fd, pipe_buff, BUFF_SIZE);

            /* exception handling: exit */
            EXCEPTION_HANDLE(res, "pipe recv handle", "error", "");

            /* have received data, clear the end */
            pipe_buff[res] = '\0';

            /* write socket */
            send(thiz->sock_fd, pipe_buff, res, 0);

            /* exception handling: warning */
            EXCEPTION_HANDLE(res, "socket write handle", "error", "warning");

            EXIT_HANDLE(pipe_buff);
        }
    }
exit:
    free(pipe_buff);
    free(sock_buff);
}

static void tcpclient_thread_entry(void *param)
{
    rt_tcpclient_t *temp = param;
    char *pipe_buff = RT_NULL, *sock_buff = RT_NULL;

    pipe_buff = malloc(BUFF_SIZE);
    if (pipe_buff == RT_NULL)
    {
        elog_e("thread entry", "malloc error\n");
        elog_i("thread entry", "exit\n");
        return;
    }

    sock_buff = malloc(BUFF_SIZE);
    if (sock_buff == RT_NULL)
    {
        free(pipe_buff);
        elog_e("thread entry", "malloc error\n");
        elog_i("thread entry", "exit\n");
        return;
    }

    memset(sock_buff, 0, BUFF_SIZE);
    memset(pipe_buff, 0, BUFF_SIZE);

    select_handle(temp, pipe_buff, sock_buff);
}

rt_tcpclient_t *rt_tcpclient_start(const char *hostname, rt_uint32_t port)
{
    rt_tcpclient_t *thiz = RT_NULL;

    thiz = tcpclient_create();
    if (thiz == RT_NULL)
        return RT_NULL;

    if (socket_init(thiz, hostname, port) != 0)
        goto quit;

    if (pipe_init(thiz) != 0)
        goto quit;

    if (tcpclient_thread_init(thiz) != 0)
        goto quit;

    elog_i("tcpcient start", "tcpclient start succeed\n");
    return thiz;

quit:
    tcpclient_destory(thiz);
    return RT_NULL;
}

void rt_tcpclient_close(rt_tcpclient_t *thiz)
{
    if (thiz == RT_NULL)
    {
        elog_e("tcpclient deinit", "param is NULL, tcpclient deinit failed\n");
        return;
    }

    if (socket_deinit(thiz) != 0)
        return;

    if (pipe_deinit(thiz) != 0)
        return;

    if (tcpclient_destory(thiz) != 0)
        return;

    elog_i("tcpclient deinit", "tcpclient deinit succeed\n");
}

rt_int32_t rt_tcpclient_send(rt_tcpclient_t *thiz, const void *buff, rt_size_t len)
{
    rt_size_t bytes = 0;

    if (thiz == RT_NULL)
    {
        elog_e("tcpclient send", "param is NULL\n");
        return -1;
    }

    if (buff == RT_NULL)
    {
        elog_e("tcpclient send", "buff is NULL\n");
        return -1;
    }

    bytes = write(thiz->pipe_write_fd, buff, len);
    return bytes;
}

rt_int32_t rt_tcpclient_attach_rx_cb(rt_tcpclient_t *thiz, rx_cb_t cb)
{
    if (thiz == RT_NULL)
    {
        elog_e("callback attach", "param is NULL\n");
        return -1;
    }

    thiz->rx = cb;
    elog_i("callback attach", "attach succeed\n");
    return 0;
}

int easy_log_init(void)
{
    /* initialize EasyFlash and EasyLogger */
    elog_init();
    /* set enabled format */
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
    /* start EasyLogger */
    elog_start();
}
INIT_APP_EXPORT(easy_log_init);