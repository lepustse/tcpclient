/*
 * File      : tcpclient_test.c
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
 * 2018-10-24     never        fix warnings
 */

#include <rtthread.h>
#include <elog.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tcpclient.h"

static rt_event_t tc_event = RT_NULL;
static rt_thread_t tid1 = RT_NULL;
static rt_thread_t tid2 = RT_NULL;

#define TC_TCPCLIENT_CLOSE (1 << 0)
#define TC_EXIT_THREAD (1 << 1)
#define TC_SWITCH_TX (1 << 2)
#define TC_SWITCH_RX (1 << 3)
#define STRCMP(a, R, b) (strcmp((a), (b)) R 0)

static void rt_tc_test_deinit(rt_tcpclient_t *thiz);

static void rt_tc_rx_cb(void *buff, rt_size_t len)
{
    char *recv = RT_NULL;

    recv = malloc(len + 1);
    if (recv == RT_NULL)
        return;

    memcpy(recv, buff, len);
    *(recv + len) = '\0';

    elog_d("tc_rx_cb", "recv data: %s\n", recv);
    elog_d("tc_rx_cb", "recv len: %d\n", strlen(recv));

    if (STRCMP(recv, ==, "exit"))
    {
        printf("receive [exit]\n");
        rt_event_send(tc_event, TC_TCPCLIENT_CLOSE);
    }

    if (STRCMP(recv, ==, "TCP disconnect"))
    {
        printf("[TCP disconnect]\n");
        rt_event_send(tc_event, TC_TCPCLIENT_CLOSE);
    }

    free(recv);
}

static void rt_tc_thread1_entry(void *param)
{
    rt_tcpclient_t *temp = param;
    const char *str = "this is thread1\r\n";
    rt_uint32_t e = 0;

    while (1)
    {
        if (rt_event_recv(tc_event, TC_TCPCLIENT_CLOSE,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          0, &e) == RT_EOK)
        {
            rt_event_send(tc_event, TC_EXIT_THREAD);
            rt_tc_test_deinit((rt_tcpclient_t *)param);
            return;
        }
        rt_thread_mdelay(5000);
        rt_tcpclient_send(temp, str, strlen(str));
    }
}

static void rt_tc_thread2_entry(void *param)
{
    rt_tcpclient_t *temp = param;
    const char *str = "this is thread2\r\n";
    rt_uint32_t e = 0;

    while (1)
    {
        if (rt_event_recv(tc_event, TC_EXIT_THREAD,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          0, &e) == RT_EOK)
        {
            rt_event_delete(tc_event);
            return;
        }

        rt_thread_mdelay(5000);
        rt_tcpclient_send(temp, str, strlen(str));
    }
}

static void rt_tc_test_deinit(rt_tcpclient_t *thiz)
{
    rt_tcpclient_close(thiz);
}

static int rt_tc_test_init(void)
{
    rt_tcpclient_t *handle = RT_NULL;

    /* 服务器的 ip 地址 & 服务器监听的端口号 */
    handle = rt_tcpclient_start("192.168.12.53", 9008);
    if (handle == RT_NULL)
    {
        elog_e("tcpclient thread", "param is NULL, exit\n");
        return -1;
    }

    /* 注册接收回调函数 */
    rt_tcpclient_attach_rx_cb(handle, rt_tc_rx_cb);

    tc_event = rt_event_create("tcev", RT_IPC_FLAG_FIFO);
    if (tc_event == RT_NULL)
    {
        elog_e("tcpclient event", "event create failed\n");
        return -1;
    }

    tid1 = rt_thread_create("tcth1", rt_tc_thread1_entry, handle, 1024, 10, 10);
    if (tid1 == RT_NULL)
    {
        elog_e("tcpclient thread1", "thread1 init failed\n");
        return -1;
    }
    rt_thread_startup(tid1);

    tid2 = rt_thread_create("tcth2", rt_tc_thread2_entry, handle, 1024, 10, 10);
    if (tid2 == RT_NULL)
    {
        elog_e("tcpclient thread2", "thread2 init failed\n");
        return -1;
    }
    rt_thread_startup(tid2);

    elog_i("tc test", "thread init succeed\n");

    return 0;
}
MSH_CMD_EXPORT(rt_tc_test_init, tc);
