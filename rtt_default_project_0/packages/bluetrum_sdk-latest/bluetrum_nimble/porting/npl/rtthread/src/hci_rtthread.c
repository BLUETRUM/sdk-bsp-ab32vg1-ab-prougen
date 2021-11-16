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

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <rtthread.h>
#include "syscfg/syscfg.h"
#include "sysinit/sysinit.h"
#include "os/os_mempool.h"
#include "nimble/ble.h"
#include "nimble/ble_hci_trans.h"
#include "nimble/hci_common.h"

void hct_tx_done(void);
void my_printf(const char *format, ...);
void my_print_r(const void *buf, uint16_t cnt);

/* HCI packet types */
#define HCI_PKT_CMD     0x01
#define HCI_PKT_ACL     0x02
#define HCI_PKT_EVT     0x04
#define HCI_PKT_GTL     0x05

/* Buffers for HCI commands data */
RT_SECTION(".btmem") static uint8_t trans_buf_cmd[BLE_HCI_TRANS_CMD_SZ];
RT_SECTION(".btmem") static uint8_t trans_buf_cmd_allocd;

/* Buffers for HCI events data */
RT_SECTION(".btmem") static uint8_t trans_buf_evt_hi_pool_buf[ OS_MEMPOOL_BYTES(
                                            MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
                                            MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE)) ];
RT_SECTION(".btmem") static struct os_mempool trans_buf_evt_hi_pool;
RT_SECTION(".btmem") static uint8_t trans_buf_evt_lo_pool_buf[ OS_MEMPOOL_BYTES(
                                                MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
                                                MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE)) ];
RT_SECTION(".btmem") static struct os_mempool trans_buf_evt_lo_pool;

/* Buffers for HCI ACL data */
#define ACL_POOL_BLOCK_SIZE OS_ALIGN(MYNEWT_VAL(BLE_ACL_BUF_SIZE) + \
                                            BLE_MBUF_MEMBLOCK_OVERHEAD + \
                                            BLE_HCI_DATA_HDR_SZ, OS_ALIGNMENT)
RT_SECTION(".btmem") static uint8_t trans_buf_acl_pool_buf[ OS_MEMPOOL_BYTES(
                                            MYNEWT_VAL(BLE_ACL_BUF_COUNT),
                                            ACL_POOL_BLOCK_SIZE) ]; //TODO 这里占了很大空间看看能不能优化
RT_SECTION(".btmem") static struct os_mempool trans_buf_acl_pool;
RT_SECTION(".btmem") static struct os_mbuf_pool trans_buf_acl_mbuf_pool;

/* Host interface */
static ble_hci_trans_rx_cmd_fn *ble_hci_rtt_rx_cmd_hs_cb;
static void *ble_hci_rtt_rx_cmd_hs_arg;
static ble_hci_trans_rx_acl_fn *ble_hci_rtt_rx_acl_hs_cb;
static void *ble_hci_rtt_rx_acl_hs_arg;

/* Called by NimBLE host to reset HCI transport state (i.e. on host reset) */
int
ble_hci_trans_reset(void)
{
    return 0;
}

/* Called by NimBLE host to setup callbacks from HCI transport */
void
ble_hci_trans_cfg_hs(ble_hci_trans_rx_cmd_fn *cmd_cb, void *cmd_arg,
                     ble_hci_trans_rx_acl_fn *acl_cb, void *acl_arg)
{
    ble_hci_rtt_rx_cmd_hs_cb = cmd_cb;
    ble_hci_rtt_rx_cmd_hs_arg = cmd_arg;
    ble_hci_rtt_rx_acl_hs_cb = acl_cb;
    ble_hci_rtt_rx_acl_hs_arg = acl_arg;
}

/*
 * Called by NimBLE host to allocate buffer for HCI Command packet.
 * Called by HCI transport to allocate buffer for HCI Event packet.
 */
uint8_t *
ble_hci_trans_buf_alloc(int type)
{
    uint8_t *buf;

    switch (type) {
    case BLE_HCI_TRANS_BUF_CMD:
        assert(!trans_buf_cmd_allocd);
        trans_buf_cmd_allocd = 1;
        buf = trans_buf_cmd;
        break;
    case BLE_HCI_TRANS_BUF_EVT_HI:
        buf = os_memblock_get(&trans_buf_evt_hi_pool);
        if (buf) {
            break;
        }
        /* no break */
    case BLE_HCI_TRANS_BUF_EVT_LO:
        buf = os_memblock_get(&trans_buf_evt_lo_pool);
        break;
    default:
        assert(0);
        buf = NULL;
    }

    return buf;
}

/*
 * Called by NimBLE host to free buffer allocated for HCI Event packet.
 * Called by HCI transport to free buffer allocated for HCI Command packet.
 */
void
ble_hci_trans_buf_free(uint8_t *buf)
{
    int rc;

    if (buf == trans_buf_cmd) {
        assert(trans_buf_cmd_allocd);
        trans_buf_cmd_allocd = 0;
    } else if (os_memblock_from(&trans_buf_evt_hi_pool, buf)) {
        rc = os_memblock_put(&trans_buf_evt_hi_pool, buf);
        assert(rc == 0);
    } else {
        assert(os_memblock_from(&trans_buf_evt_lo_pool, buf));
        rc = os_memblock_put(&trans_buf_evt_lo_pool, buf);
        assert(rc == 0);
    }
}

void hct_send_command(uint16_t opcode, uint8_t len, uint8_t pbuf[]);
/* Called by NimBLE host to send HCI Command packet over HCI transport */
int ble_hci_trans_hs_cmd_tx(uint8_t *cmd)
{
    int rc = 0;
    struct ble_hci_cmd *cmdp = (void *)cmd;

    assert(ble_hci_rtt_rx_cmd_hs_cb != NULL);

    my_printf("CMD => %x %x %x ", cmdp->opcode & 0xff, cmdp->opcode >> 8, cmdp->length);
    my_print_r(cmdp->data, cmdp->length);
    hct_send_command(cmdp->opcode, cmdp->length, cmdp->data);

    ble_hci_trans_buf_free(cmd);

    return rc;
}

bool hct_acl_segment(uint16_t handel, uint8_t flags, uint16_t len, uint8_t pbuf[]);
/* Called by NimBLE host to send HCI ACL Data packet over HCI transport */
int ble_hci_trans_hs_acl_tx(struct os_mbuf *om)
{
    uint8_t *buf = om->om_data;
    uint16_t len = om->om_len;

    uint16_t handle = buf[0] | ((buf[1] & 0xf) << 8);
    uint8_t  flags = (buf[1] & 0xf0) >> 4;
    uint16_t acl_len = buf[2] | (buf[3] << 8);

    my_printf("ACL => ");
    my_print_r(buf, len);
    hct_acl_segment(handle, flags, acl_len, buf + 4);
    os_mbuf_free_chain(om);

    return 0;
}

/* Called by application to send HCI Event packet to host */
int hci_transport_send_evt_to_host(uint8_t *buf, uint8_t size)
{
    uint8_t *trans_buf;
    int rc;

    /* Allocate LE Advertising Report Event from lo pool only */
    if ((buf[0] == BLE_HCI_EVCODE_LE_META) &&
        (buf[2] == BLE_HCI_LE_SUBEV_ADV_RPT)) {
        trans_buf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_LO);
        if (!trans_buf) {
            /* Skip advertising report if we're out of memory */
            return 0;
        }
    } else {
        trans_buf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_HI);
    }

    memcpy(trans_buf, buf, size);

    rc = ble_hci_rtt_rx_cmd_hs_cb(trans_buf, ble_hci_rtt_rx_cmd_hs_arg);
    hct_tx_done();

    if (rc != 0) {
        ble_hci_trans_buf_free(trans_buf);
    }

    return rc;
}

/* Called by application to send HCI ACL Data packet to host */
int
hci_transport_send_acl_to_host(uint8_t *buf, uint16_t size)
{
    struct os_mbuf *trans_mbuf;
    int rc;

    trans_mbuf = os_mbuf_get_pkthdr(&trans_buf_acl_mbuf_pool,
                                    sizeof(struct ble_mbuf_hdr));
    os_mbuf_append(trans_mbuf, buf, size);
    rc = ble_hci_rtt_rx_acl_hs_cb(trans_mbuf, ble_hci_rtt_rx_acl_hs_arg);
    hct_tx_done();

    return rc;
}

/* Called by application to initialize transport structures */
int ble_hci_rtthread_init(void)
{
    int rc;

    trans_buf_cmd_allocd = 0;

    rc = os_mempool_init(&trans_buf_acl_pool, MYNEWT_VAL(BLE_ACL_BUF_COUNT),
                                ACL_POOL_BLOCK_SIZE, trans_buf_acl_pool_buf,
                                "dummy_hci_acl_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mbuf_pool_init(&trans_buf_acl_mbuf_pool, &trans_buf_acl_pool,
                                ACL_POOL_BLOCK_SIZE,
                                MYNEWT_VAL(BLE_ACL_BUF_COUNT));
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_init(&trans_buf_evt_hi_pool,
                                MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
                                MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE),
                                trans_buf_evt_hi_pool_buf,
                                "dummy_hci_hci_evt_hi_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_init(&trans_buf_evt_lo_pool,
                                MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
                                MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE),
                                trans_buf_evt_lo_pool_buf,
                                "dummy_hci_hci_evt_lo_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    return 0;
}
