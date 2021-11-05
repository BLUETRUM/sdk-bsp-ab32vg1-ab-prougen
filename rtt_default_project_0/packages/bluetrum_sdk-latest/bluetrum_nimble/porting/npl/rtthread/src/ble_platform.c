/*
 * Copyright (c) 2006-2021, Bluetrum Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-15     ZhuHao       the first version
 */

// Only for Bluetrum

#include <rtthread.h>
#include <rthw.h>
#include <stdint.h>
#include <string.h>

#include <rtthread.h>
#include "ab32vgx.h"

void bthw_soft_isr(void);
void bthw_isr_do_plus(void);
void my_printf(const char *format, ...);
void my_print_r(const void *buf, uint16_t cnt);

static volatile uint32_t bthw_soft_flag = 0;
static rt_sem_t psem_bthw = RT_NULL;
static rt_sem_t psem_btctrl = RT_NULL;

void delay_5ms(uint32_t n)
{
    rt_thread_mdelay(5*n);
}

typedef void (*isr_t)(void);
static isr_t irq_func = RT_NULL;

RT_SECTION(".com_text.irq")
static void irq_wrapper(int vector, void *param)
{
    rt_interrupt_enter();
    if (irq_func != RT_NULL)
        irq_func();
    rt_interrupt_leave();
}

isr_t register_isr(int vector, isr_t isr)
{
    irq_func = isr;
    rt_hw_interrupt_install(vector, irq_wrapper, RT_NULL, "bthw");
}

int hci_transport_send_acl_to_host(uint8_t *buf, uint16_t size);
int hci_transport_send_evt_to_host(uint8_t *buf, uint8_t size);
RT_SECTION(".com_text.stack.hci_recv")
void hci_host_recv_packet(uint8_t *buf, int len)
{
    switch (buf[0]) {
        case 0x2:
            rt_kprintf("ACL <= ");
            my_print_r(buf+1, len-1);
            hci_transport_send_acl_to_host(buf+1, len-1);
            break;

        case 0x4:
            rt_kprintf("EVT <= ");
            my_print_r(buf+1, len-1);
            hci_transport_send_evt_to_host(buf+1, len-1);
            break;
    }
}

void bt_get_local_bd_addr(uint8_t *addr)
{
    uint8_t addr_table[6] = {0x41, 0x42, 0x00, 0x00, 0x00};
    memcpy(addr, addr_table, 6);
}

/* btctrl thread */

RT_SECTION(".com_text")
void nanos_event_set_trigger(void)
{
    rt_sem_release(psem_btctrl);
}

void bb_run_loop(void);
RT_SECTION(".com_text")
static void btctrl_thread_entry(void *param)
{
    while (1) {
        rt_sem_take(psem_btctrl, RT_WAITING_FOREVER);
        bb_run_loop();
    }
}

static int btctrl_thread_init(void)
{
    psem_btctrl = rt_sem_create("bb", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t pid = rt_thread_create(
        "bb",
        btctrl_thread_entry,
        RT_NULL,
        1024,
        6,
        1
    );

    if (pid != RT_NULL) {
        rt_thread_startup(pid);
    }
}
INIT_APP_EXPORT(btctrl_thread_init);

/* bthw thread */

RT_SECTION(".com_text")
void bthw_thread_post(void)
{
    rt_sem_release(psem_bthw);
}

RT_SECTION(".com_text")
void bthw_soft_kick(void)
{
    bthw_soft_flag = 1;
    rt_sem_release(psem_bthw);
}

RT_SECTION(".com_text")
static void bthw_thread_entry(void *param)
{
    bthw_soft_flag = 0;

    while(1) {
        rt_sem_take(psem_bthw, RT_WAITING_FOREVER);
        if (bthw_soft_flag) {
            bthw_soft_flag = 0;
            bthw_soft_isr();
        } else {
            bthw_isr_do_plus();
        }
    }
}

void bthw_irq_init(void);
static int bthw_thread_init(void)
{
    bthw_irq_init();
    psem_bthw = rt_sem_create("bthw", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t tid = rt_thread_create(
        "bthw",
        bthw_thread_entry,
        RT_NULL,
        1024,
        5,
        1
    );

    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    }

    return 0;
}
INIT_APP_EXPORT(bthw_thread_init);
