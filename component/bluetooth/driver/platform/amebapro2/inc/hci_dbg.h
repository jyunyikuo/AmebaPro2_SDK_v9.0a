/*
 *******************************************************************************
 * Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 */

#ifndef _HCI_DBG_H_
#define _HCI_DBG_H_

#include "hci/hci_common.h"

#define EFUSE_SW_USE_FLASH_PATCH   BIT0
#define EFUSE_SW_BT_FW_LOG         BIT1
#define EFUSE_SW_RSVD              BIT2
#define EFUSE_SW_IQK_HCI_OUT       BIT3
#define EFUSE_SW_UPPERSTACK_SWITCH BIT4
#define EFUSE_SW_TRACE_SWITCH      BIT5
#define EFUSE_SW_DRIVER_DEBUG_LOG  BIT6
#define EFUSE_SW_RSVD2             BIT7

extern uint32_t hci_cfg_sw_val;
//#define FLASH_BT_PARA_ADDR       (SYS_DATA_FLASH_BASE + 0xFF0)
//#define READ_SW(sw)              (sw = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR))
#define CHECK_CFG_SW(x)            (hci_cfg_sw_val & x)

enum hci_dbg_sw {
    HCI_TP_DEBUG_ERROR,
    HCI_TP_DEBUG_WARN,
    HCI_TP_DEBUG_INFO,
    HCI_TP_DEBUG_DEBUG,
    HCI_TP_DEBUG_HCI_UART_TX,
    HCI_TP_DEBUG_HCI_UART_RX,
    HCI_TP_DEBUG_HCI_UART_RX_IDX,
    HCI_TP_DEBUG_DOWNLOAD_PATCH,
    HCI_TP_DEBUG_HCI_STACK_DEBUG,
};

void     hci_dbg_printf    (const char* fmt, ...);
void     hci_dbg_set_level (uint32_t level);
uint32_t hci_dbg_get_level (void);

#define H_BIT(x)           (1 << (x))
#define HCI_DEBUG_ALL      (H_BIT(HCI_TP_DEBUG_DEBUG) | \
                            H_BIT(HCI_TP_DEBUG_INFO)  | \
                            H_BIT(HCI_TP_DEBUG_WARN)  | \
                            H_BIT(HCI_TP_DEBUG_ERROR) ) //0xFFFFFFFF
#define CHECK_DBG_SW(x)    (hci_dbg_get_level() & H_BIT(x))

#define HCI_ASSERT(...)    \
    do                     \
    {                      \
    } while (0)

#define HCI_ERR(fmt, ...)                                                               \
    do                                                                                  \
    {                                                                                   \
        hci_dbg_printf("%s:%d(err) " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);      \
    } while (0)

#define HCI_DBG(fmt, ...)                                                               \
    do                                                                                  \
    {                                                                                   \
        if (CHECK_DBG_SW(HCI_TP_DEBUG_DEBUG))                                           \
            hci_dbg_printf("%s:%d(dbg) " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    } while (0)

#define HCI_INFO(fmt, ...)                                                              \
    do                                                                                  \
    {                                                                                   \
        if (CHECK_DBG_SW(HCI_TP_DEBUG_INFO))                                            \
            hci_dbg_printf("%s:%d(info) " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define HCI_WARN(fmt, ...)                                                              \
    do                                                                                  \
    {                                                                                   \
        if (CHECK_DBG_SW(HCI_TP_DEBUG_WARN))                                            \
            hci_dbg_printf("%s:%d(warn) " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define HCI_STACK_DBG(fmt, ...)                                                         \
    do                                                                                  \
    {                                                                                   \
        if (CHECK_DBG_SW(HCI_TP_DEBUG_HCI_STACK_DEBUG))                                 \
            hci_dbg_printf("%s:%d[dbg] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    } while (0)

#define HCI_DUMP(hdr, hdr_len, data, data_len) \
    do                                         \
    {                                          \
    } while (0)

#endif