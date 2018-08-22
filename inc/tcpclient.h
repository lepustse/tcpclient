/*
 * File      : tcpclient.h
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
#ifndef __TCPCLIENT_H__
#define __TCPCLIENT_H__

#include <rtthread.h>

typedef void (*rx_cb_t)(void *buff, rt_size_t len);
typedef struct rt_tcpclient rt_tcpclient_t;

rt_tcpclient_t *rt_tcpclient_start(const char *hostname, rt_uint32_t port);
void rt_tcpclient_close(rt_tcpclient_t *thiz);
rt_int32_t rt_tcpclient_attach_rx_cb(rt_tcpclient_t *thiz, rx_cb_t cb);
rt_int32_t rt_tcpclient_send(rt_tcpclient_t *thiz, const void *buff, rt_size_t len);

#endif