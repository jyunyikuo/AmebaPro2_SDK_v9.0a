/**************************************************************************//**
 * @file     hal_uart.c
 * @brief    This UART HAL API functions.
 *
 * @version  V1.00
 * @date     2021-05-20
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/
#include "cmsis.h"
#include "hal_uart.h"
#include "hal_gdma.h"
#include "hal_pinmux.h"
#include "hal_cache.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "memory.h"
#include "hal_uart_nsc.h"
#include "hal_sys_ctrl.h"
#include "hal_sys_ctrl_nsc.h"

#if CONFIG_UART_EN

//extern hal_status_t hal_gpio_pull_ctrl (uint32_t pin_name, pin_pull_type_t pull_type);

/**
 * @addtogroup hal_uart UART
 * @{
 * @brief The UART HAL APIs.
 */

#define UART_GDMA_MAX_BLOCK         16
gdma_ch_lli_t uart_tx_gdma_ch_lli[MAX_UART_PORT][UART_GDMA_MAX_BLOCK] __ALIGNED(32);
gdma_ch_lli_t uart_rx_gdma_ch_lli[MAX_UART_PORT][UART_GDMA_MAX_BLOCK] __ALIGNED(32);


uint8_t hal_uart_check_uart_id(uint32_t tx_pin, uint32_t rx_pin)
{
	uint8_t uart_idx = NONESET_UART_IDX;

#if !IS_AFTER_CUT_C(CONFIG_CHIP_VER)
	//To fix UART4 idx erro in ROM code
	if (tx_pin == PIN_UART4_TX) {
		if (rx_pin == PIN_UART4_RX) {
			return Uart4;
		}
	}
#endif

	if (tx_pin != PIN_NC) {
		uart_idx = hal_uart_stubs.hal_uart_pin_to_idx(tx_pin, UART_Pin_TX);
		if (uart_idx >= MAX_UART_PORT) {
			DBG_UART_ERR("%s: pin(0x%x) is not for UART TX\r\n", __func__, tx_pin);
			return NONESET_UART_IDX;
		}
	}

	if (rx_pin != PIN_NC) {
		if (uart_idx != 0xFF) {
			if (uart_idx != hal_uart_stubs.hal_uart_pin_to_idx(rx_pin, UART_Pin_RX)) {
				DBG_UART_ERR("%s:tx_pin(0x%x) & rx_pin(0x%x) is not on the same UART\r\n", __func__, tx_pin, rx_pin);
				return NONESET_UART_IDX;
			}
		} else {
			uart_idx = hal_uart_stubs.hal_uart_pin_to_idx(rx_pin, UART_Pin_RX);
			if (uart_idx >= MAX_UART_PORT) {
				DBG_UART_ERR("%s: pin(0x%x) is not for UART RX\r\n", __func__, rx_pin);
				return NONESET_UART_IDX;
			}
		}
	}

	if (uart_idx >= MaxUartNum) {
		return NONESET_UART_IDX;
	}
	return uart_idx;
}

/**
 *  @brief To initial a UART port adapter. This function must be called before any UART port
 *         operation. This function will do:
 *           - enable the UART hardware.
 *           - register the interrupt handler.
 *           - configures the pin mux.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *  @param[in]  tx_pin   The UART TX pin name.
 *  @param[in]  rx_pin   The UART RX pin name.
 *  @param[in]  pin_sel  The pin mux selection.
 *  @param[in]  pconfig  The extra UART port setting for the initial configuration.
 *                       This is an UART adapter initial value. If this value is not NULL,
 *                       the initialization function will initial the new UART adapter by
 *                       this initial value. And also will do further configure, configures
 *                       the bard rate, hardware flow control and the frame format.
 *
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_OK:  UART port initialization OK.
 */
hal_status_t hal_uart_init(phal_uart_adapter_t puart_adapter, uint32_t tx_pin, uint32_t rx_pin,
						   phal_uart_defconfig_t pconfig)
{
	hal_status_t ret;
	uint8_t uart_idx;

	if ((rx_pin != PIN_NC) && (rx_pin != PIN_UART4_RX)) {
		// RX Pin pull-high to prevent this folating on this pin
		hal_gpio_pull_ctrl(rx_pin, Pin_PullUp);
		hal_delay_us(4);
	}

	uart_idx = hal_uart_check_uart_id(tx_pin, rx_pin);
	if ((uart_idx < MaxUartNum) && (Uart0 == uart_idx)) {
		//makesure buf is clean
		hal_uart_en_ctrl(Uart0, OFF);
	}

#if IS_AFTER_CUT_C(CONFIG_CHIP_VER)
	ret = hal_uart_stubs.hal_uart_init(puart_adapter, tx_pin, rx_pin, pconfig);
#else
	//To fix UART4 idx error in ROM code
	if (uart_idx == Uart4) {
		ret = hal_rtl_uart_init_bt_patch(puart_adapter);
	} else {
		ret = hal_uart_stubs.hal_uart_init(puart_adapter, tx_pin, rx_pin, pconfig);
	}
#endif  //#if IS_AFTER_CUT_C(CONFIG_CHIP_VER)

#if defined(CONFIG_BUILD_NONSECURE)
	/* Only NS flow */
	if (Uart0 == puart_adapter->uart_idx) {
		//UART0 default SCLK= 40MHz
		hal_sys_set_clk(UART0_SYS, UART_PERI_40M);
	}

	/* hook sys ctl fun ptr */
	puart_adapter->hal_sys_peripheral_en_cb = hal_uart_en_ctrl;

	/* Enable func/bus/pclk/sclk */
	hal_uart_en_ctrl(puart_adapter->uart_idx, ON);
	ret = hal_uart_stubs.hal_uart_load_default_state(puart_adapter, pconfig);
#endif

#if !CONFIG_PXP
	//PXP ROM already fixed using clk src = 40M, baud rate = 115200, so ignore below 40M->4M setting
	//set UART0 SCLK= 4MHz
	if (Uart0 == puart_adapter->uart_idx) {
		hal_uart_lp_sclk_select(puart_adapter, UART_IRC_4M);
		//calibration OSC 4M
		hal_osc4m_cal();
	}
#endif
	//pinmux control setting
	if (ret == HAL_OK) {
		uart_idx = puart_adapter->uart_idx;
		if (uart_idx <= Uart3) {
			// only UART0,1,2,3 has real IO pin
			if (tx_pin != PIN_NC) {
				ret = hal_pinmux_register(tx_pin, (PID_UART0 + uart_idx));
			}

			if (rx_pin != PIN_NC) {
				ret |= hal_pinmux_register(rx_pin, (PID_UART0 + uart_idx));
			}

		} else if (uart_idx == Uart4) {
			/* BT UART MUX selection*/
			hal_sys_bt_uart_mux(BT_UART_MUX_INTERNAL);
		}
	} else {
		if (rx_pin != PIN_NC) {
			hal_gpio_pull_ctrl(rx_pin, Pin_PullNone);
		}
	}
	return ret;
}

#if 0 //IS_CUT_TEST(CONFIG_CHIP_VER) // & defined(CONFIG_BUILD_NONSECURE)
hal_status_t hal_uart_init_for_bt(phal_uart_adapter_t puart_adapter)
{
	uint8_t uart_idx = 4;
	hal_status_t ret = HAL_OK;

	hal_uart_stubs.hal_uart_adapter_init(puart_adapter, (uint8_t)uart_idx, NULL);
	puart_adapter->tx_pin = PIN_NC;
	puart_adapter->rx_pin = PIN_NC;
	puart_adapter->rts_pin = PIN_NC;
	puart_adapter->cts_pin = PIN_NC;

	/* Register UART IRQ handler*/
	hal_uart_stubs.hal_uart_reg_irq(puart_adapter, (irq_handler_t)hal_uart_stubs.uart_irq_handler);
	/* Enable func/bus/pclk/sclk */
	hal_uart_en_ctrl(puart_adapter->uart_idx, ON);
	hal_uart_stubs.hal_uart_load_default_state(puart_adapter, NULL);
	/* BT UART MUX selection*/
	hal_sys_bt_uart_mux(BT_UART_MUX_INTERNAL);
	return ret;
}
#endif

/**
 *  @brief Disable the given UART port. It will do:
 *           - disable UART hardware function.
 *           - disable UART GDMA channel.
 *           - disable UART pins.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *
 *  @returns void
 */
void hal_uart_deinit(phal_uart_adapter_t puart_adapter)
{
	uint32_t uart_idx = puart_adapter->uart_idx;

	hal_uart_stubs.hal_uart_deinit(puart_adapter);

	if (uart_idx <= Uart3) {
		if (puart_adapter->tx_pin != PIN_NC) {
			hal_pinmux_unregister(puart_adapter->tx_pin, (PID_UART0 + uart_idx));
		}

		if (puart_adapter->rx_pin != PIN_NC) {
			hal_pinmux_unregister(puart_adapter->rx_pin, (PID_UART0 + uart_idx));
			hal_gpio_pull_ctrl(puart_adapter->rx_pin, Pin_PullNone);
		}

		if (puart_adapter->rts_pin != PIN_NC) {
			hal_pinmux_unregister(puart_adapter->rts_pin, (PID_UART0 + uart_idx));
		}

		if (puart_adapter->cts_pin != PIN_NC) {
			hal_pinmux_unregister(puart_adapter->cts_pin, (PID_UART0 + uart_idx));
		}
	}
}

/**
 *  @brief Select low-power UART SCLK, and setting default baud-rate table.
 *
 *  @param[in]  puart_adapter   The UART adapter.
 *  @param[in]  sclk_sel   The low-power UART SCLK source.
 *
 *  @returns    The UART index. If the given pin name didn't map to a valid UART, the return value is 0xFF.
 */
#if !defined(CONFIG_BUILD_NONSECURE)
//for ntz & s
hal_status_t hal_uart_lp_sclk_select(phal_uart_adapter_t puart_adapter, uint8_t sclk_sel)
{
	return hal_uart_stubs.hal_uart_lp_sclk_select(puart_adapter, sclk_sel);
}
#endif

/**
 *  @brief Configures the UART hardware auto flow-control setting.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *  @param[in]  flow_ctrl  The flow control setting.
 *                           - 0: no hardware flow control.
 *                           - 1: enable RX (RTS) flow control.
 *                           - 2: enable TX (CTS) flow control.
 *                           - 3: enable RTS and CTS hardware flow control.
 *
 *  @returns    Always return HAL_OK
 */
hal_status_t hal_uart_set_flow_control(phal_uart_adapter_t puart_adapter, uint32_t flow_ctrl)
{
	uint8_t uart_idx = puart_adapter->uart_idx;
	hal_status_t ret;

	ret = hal_uart_stubs.hal_uart_set_flow_control(puart_adapter, flow_ctrl);

	if (uart_idx <= Uart4) {
		switch (flow_ctrl) {
		case UartFlowCtlNone:
			if (puart_adapter->rts_pin != PIN_NC) {
				ret |= hal_pinmux_unregister(puart_adapter->rts_pin, (PID_UART0 + uart_idx));
				puart_adapter->rts_pin = PIN_NC;
			}

			if (puart_adapter->cts_pin != PIN_NC) {
				ret |= hal_pinmux_unregister(puart_adapter->cts_pin, (PID_UART0 + uart_idx));
				puart_adapter->cts_pin = PIN_NC;
			}
			break;

		case UartFlowCtlRTSCTS:
			ret |= hal_pinmux_register(puart_adapter->rts_pin, (PID_UART0 + uart_idx));
			ret |= hal_pinmux_register(puart_adapter->cts_pin, (PID_UART0 + uart_idx));
			break;

		case UartFlowCtlRTS:
			ret |= hal_pinmux_register(puart_adapter->rts_pin, (PID_UART0 + uart_idx));
			if (puart_adapter->cts_pin != PIN_NC) {
				ret |= hal_pinmux_unregister(puart_adapter->cts_pin, (PID_UART0 + uart_idx));
				puart_adapter->cts_pin = PIN_NC;
			}
			break;

		case UartFlowCtlCTS:
			ret |= hal_pinmux_register(puart_adapter->cts_pin, (PID_UART0 + uart_idx));
			if (puart_adapter->rts_pin != PIN_NC) {
				ret |= hal_pinmux_unregister(puart_adapter->rts_pin, (PID_UART0 + uart_idx));
				puart_adapter->rts_pin = PIN_NC;
			}
			break;

		default:
			break;
		}
	}
	return ret;
}

/**
 *  @brief To initial a GDMA channel for the UART TX DMA mode transfer.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *  @param[in]  pgdma_chnl The GDMA channel adapter. It is use to control
 *              the GDMA channel transfer.
 *
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_OK:  GDMA channel initialization OK.
 */
hal_status_t hal_uart_tx_gdma_init(phal_uart_adapter_t puart_adapter, phal_gdma_adaptor_t pgdma_chnl)
{
	hal_status_t ret;

	memset((void *)pgdma_chnl, 0, sizeof(hal_gdma_adaptor_t));
	ret = hal_gdma_chnl_alloc(pgdma_chnl, MultiBlkDis);     // default no-multiple block support

	if (ret == HAL_OK) {
#if IS_CUT_TEST(CONFIG_CHIP_VER)
		//ret = hal_uart_tx_gdma_init_ram(puart_adapter, pgdma_chnl);
		ret = hal_rtl_uart_tx_gdma_init_patch(puart_adapter, pgdma_chnl);
#else
		ret = hal_uart_stubs.hal_uart_tx_gdma_init(puart_adapter, pgdma_chnl);
#endif
		if (ret == HAL_OK) {
			puart_adapter->dcache_clean_by_addr = hal_cache_stubs.dcache_clean_by_addr;
			hal_gdma_chnl_init(pgdma_chnl);
		} else {
			DBG_UART_ERR("hal_uart_tx_gdma_init: GDMA init failed(%d)\r\n", ret);
			hal_gdma_chnl_free(pgdma_chnl);
		}
	} else {
		DBG_UART_ERR("hal_uart_tx_gdma_init: GDMA channel allocate failed(%d)\r\n", ret);
	}

	return ret;
}

/**
 *  @brief To de-initial the UART TX GDMA channel.
 *         Also will disable the UART TX DMA transfer mode.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_OK:  GDMA channel de-initialization OK.
 */
hal_status_t hal_uart_tx_gdma_deinit(phal_uart_adapter_t puart_adapter)
{
	hal_gdma_chnl_free(puart_adapter->ptx_gdma);
	return hal_uart_stubs.hal_uart_tx_gdma_deinit(puart_adapter);
}

/**
 *  @brief To initial a GDMA channel for the UART RX DMA mode transfer.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *  @param[in]  pgdma_chnl The GDMA channel adapter. It is use to control
 *              the GDMA channel transfer.
 *
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_OK:  GDMA channel initialization OK.
 */
hal_status_t hal_uart_rx_gdma_init(phal_uart_adapter_t puart_adapter, phal_gdma_adaptor_t pgdma_chnl)
{
	hal_status_t ret;

	memset((void *)pgdma_chnl, 0, sizeof(hal_gdma_adaptor_t));
	ret = hal_gdma_chnl_alloc(pgdma_chnl, MultiBlkDis);     // default no-multiple block support

	if (ret == HAL_OK) {
#if IS_CUT_TEST(CONFIG_CHIP_VER)
		//ret = hal_uart_rx_gdma_init_ram(puart_adapter, pgdma_chnl);
		ret = hal_rtl_uart_rx_gdma_init_patch(puart_adapter, pgdma_chnl);
#else
		ret = hal_uart_stubs.hal_uart_rx_gdma_init(puart_adapter, pgdma_chnl);
#endif
		if (ret == HAL_OK) {
			puart_adapter->dcache_invalidate_by_addr = hal_cache_stubs.dcache_invalidate_by_addr;
			hal_gdma_chnl_init(pgdma_chnl);
		} else {
			DBG_UART_ERR("hal_uart_rx_gdma_init: GDMA init failed(%d)\r\n", ret);
			hal_gdma_chnl_free(pgdma_chnl);
		}
	} else {
		DBG_UART_ERR("hal_uart_rx_gdma_init: GDMA channel allocate failed(%d)\r\n", ret);
	}

	return ret;
}

/**
 *  @brief To de-initial the UART RX GDMA channel.
 *         Also will disable the UART RX DMA transfer mode.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_OK:  GDMA channel de-initialization OK.
 */
hal_status_t hal_uart_rx_gdma_deinit(phal_uart_adapter_t puart_adapter)
{
	hal_gdma_chnl_free(puart_adapter->prx_gdma);
	return hal_uart_stubs.hal_uart_rx_gdma_deinit(puart_adapter);
}

/**
 *  @brief To receive a block of data by the DMA mode.
 *         This function returns without waiting of data receiving done.
 *
 *  @param[in]  puart_adapter The UART adapter.
 *  @param[out] prx_buf The buffer for the data receiving.
 *  @param[in]  len  The length of data, in byte, are going to receive.
 *
 *  @return     HAL_OK: function execution OK.
 *  @return     HAL_BUSY: UART RX is in busy state, previous receiving is not finished yet.
 *  @return     HAL_ERR_PARA: Input arguments are invalid.
 *  @return     HAL_NO_RESOURCE: Multiple-block DMA channel allocation failed.
 */
hal_status_t hal_uart_dma_recv(phal_uart_adapter_t puart_adapter, uint8_t *prx_buf, uint32_t len)
{
	hal_gdma_adaptor_t *pgdma_chnl = puart_adapter->prx_gdma;
	hal_status_t ret;
	volatile uint32_t gdma_idx;

	if (pgdma_chnl == NULL) {
		DBG_UART_ERR("hal_uart_dma_recv: No GDMA Chnl\r\n");
		return HAL_NO_RESOURCE;
	}

	if (len > MAX_DMA_BLOCK_SIZE) {
		if (len <= MAX_DMA_BLOCK_SIZE * 32) {
			// Need to use multiple block DMA
			if (pgdma_chnl->ch_num < 4) {
				// Current GDMA Channel didn't support multiple block DMA, re-allocate another one
				gdma_idx = pgdma_chnl->gdma_index;  // backup old GDMA index
				hal_gdma_chnl_free(pgdma_chnl);
				ret = hal_gdma_chnl_alloc(pgdma_chnl, MultiBlkEn);
				if (ret != HAL_OK) {
					puart_adapter->prx_gdma = NULL;
					DBG_UART_ERR("hal_uart_dma_recv: Err: re-allocate multiple block DMA channel failed(%d)\r\n", ret);
					return ret;
				} else {
					DBG_UART_INFO("hal_uart_dma_recv: re-allocate GDMA %u chnl %u\r\n", pgdma_chnl->gdma_index, pgdma_chnl->ch_num);
					pgdma_chnl->pgdma_ch_lli = &uart_rx_gdma_ch_lli[puart_adapter->uart_idx][0];
					hal_gdma_chnl_init(pgdma_chnl);
				}

				// Update GDMA handshake bit and IRQ handler(since may use different GDMA HW)
				if (gdma_idx != pgdma_chnl->gdma_index) {
					// allocated to different GDMA HW, update the handshake bit
					hal_gdma_handshake_init(pgdma_chnl, pgdma_chnl->gdma_cfg.src_per);
				}
				hal_gdma_irq_reg(pgdma_chnl, (irq_handler_t)hal_uart_stubs.uart_rx_dma_irq_handler, puart_adapter);
			}

		} else {
			DBG_UART_ERR("hal_uart_dma_recv: Err: RX Len(%lu) too big\n", len);
			return HAL_ERR_PARA;
		}
	}
	return hal_uart_stubs.hal_uart_dma_recv(puart_adapter, prx_buf, len);
}

/**
 *  @brief To send a block of data by the DMA transmission mode.
 *
 *  @param[in]  puart_adapter  The UART adapter.
 *  @param[in]  ptx_buf  The buffer of data to be transmitted.
 *  @param[in]  len  The length of data in bytes to be transmitted.
 *
 *  @return     HAL_OK: function execution OK.
 *  @return     HAL_BUSY: UART TX is in busy state, previous transmission is not finished yet.
 *  @return     HAL_ERR_PARA: Input arguments are invalid.
 *  @return     HAL_NO_RESOURCE: Multiple-block DMA channel allocation failed.
 */
hal_status_t hal_uart_dma_send(phal_uart_adapter_t puart_adapter, uint8_t *ptx_buf, uint32_t len)
{
	hal_gdma_adaptor_t *pgdma_chnl = puart_adapter->ptx_gdma;
	uint32_t block_size;
	hal_status_t ret;
	volatile uint32_t gdma_idx;

	if (pgdma_chnl == NULL) {
		DBG_UART_ERR("hal_uart_dma_send: No GDMA Chnl\r\n");
		return HAL_NO_RESOURCE;
	}

	if (((len & 0x03) == 0) && (((uint32_t)(ptx_buf) & 0x03) == 0)) {
		// 4-bytes aligned, move 4 bytes each transfer
		block_size = len >> 2;
	} else {
		block_size = len;
	}

	if (block_size > MAX_DMA_BLOCK_SIZE) {
		if (block_size <= MAX_DMA_BLOCK_SIZE * 32) {
			// Need to use multiple block DMA
			if (pgdma_chnl->ch_num < 4) {
				// Current GDMA Channel didn't support multiple block DMA, re-allocate another one
				DBG_UART_INFO("hal_uart_dma_send: re-allocate GDMA chnl to support Multi-Blk transfer\r\n");
				gdma_idx = pgdma_chnl->gdma_index;  // backup old GDMA index
				hal_gdma_chnl_free(pgdma_chnl);
				ret = hal_gdma_chnl_alloc(pgdma_chnl, MultiBlkEn);

				if (ret != HAL_OK) {
					puart_adapter->ptx_gdma = NULL;
					DBG_UART_ERR("hal_uart_dma_send: Err: re-allocate multiple block DMA channel failed(%d)\r\n", ret);
					return ret;
				} else {
					DBG_UART_INFO("hal_uart_dma_send: re-allocated GDMA %u chnl %u\r\n", pgdma_chnl->gdma_index, pgdma_chnl->ch_num);
					pgdma_chnl->pgdma_ch_lli = &uart_tx_gdma_ch_lli[puart_adapter->uart_idx][0];
					hal_gdma_chnl_init(pgdma_chnl);
				}

				// Update GDMA handshake bit and IRQ handler(since may use different GDMA HW)
				if (gdma_idx != pgdma_chnl->gdma_index) {
					// allocated to different GDMA HW, update the handshake bit
					hal_gdma_handshake_init(pgdma_chnl, pgdma_chnl->gdma_cfg.dest_per);
				}
				hal_gdma_irq_reg(pgdma_chnl, (irq_handler_t)hal_uart_stubs.uart_tx_dma_irq_handler, puart_adapter);
			}
		} else {
			DBG_UART_ERR("hal_uart_dma_send: Err: TX length too big(%lu)\r\n", len);
			return HAL_ERR_PARA;
		}
	}

	return hal_uart_stubs.hal_uart_dma_send(puart_adapter, ptx_buf, len);
}
/** @} */ /* End of group hs_hal_uart */

#if 0// IS_CUT_TEST(CONFIG_CHIP_VER) //ram patch
uint8_t uart_tx_gdma_hsk_id_tbl_ram[] = {
	GDMA_HANDSHAKE_UART0_TX,        // GDMA hardware handshake number for UART0 TX
	GDMA_HANDSHAKE_UART1_TX,        // GDMA hardware handshake number for UART1 TX
	GDMA_HANDSHAKE_UART2_TX,        // GDMA hardware handshake number for UART2 TX
	GDMA_HANDSHAKE_UART3_TX,        // GDMA hardware handshake number for UART3 TX
	GDMA_HANDSHAKE_BT_UART_TX       // GDMA hardware handshake number for UART4 TX
};

uint8_t uart_rx_gdma_hsk_id_tbl_ram[] = {
	GDMA_HANDSHAKE_UART0_RX,        // GDMA hardware handshake number for UART0 RX
	GDMA_HANDSHAKE_UART1_RX,        // GDMA hardware handshake number for UART1 RX
	GDMA_HANDSHAKE_UART2_RX,        // GDMA hardware handshake number for UART2 RX
	GDMA_HANDSHAKE_UART3_RX,        // GDMA hardware handshake number for UART3 RX
	GDMA_HANDSHAKE_BT_UART_RX       // GDMA hardware handshake number for UART4 RX
};

void _uart_tx_dma_irq_handler_ram(phal_uart_adapter_t puart_adapter)
{
	UART_Type *puart = puart_adapter->base_addr;

	puart->miscr_b.txdma_en = 0; // Disable UART TX DMA

	// update TX Buffer adddr
	puart_adapter->ptx_buf = (uint8_t *)((uint32_t)(puart_adapter->ptx_buf_sar) + puart_adapter->tx_count);

	// Wait all data in TX FIFO to be transfered
	// Enable TX FIFO empty interrupt (TX totally done)
	if (puart->tflvr_b.tx_fifo_lv == 0) {
		// Call user TX complete callback
		if (NULL != puart_adapter->tx_done_callback) {
			puart_adapter->tx_done_callback(puart_adapter->tx_done_cb_para);
		}
		uart_clear_state(puart_adapter, HAL_UART_STATE_DMATX_BUSY);
	} else {
		puart->ier_b.etbei = 1;
	}
}

void _uart_rx_dma_irq_handler_ram(phal_uart_adapter_t puart_adapter)
{
	UART_Type *puart = puart_adapter->base_addr;

	puart->miscr_b.rxdma_en = 0; // Disable UART RX DMA
	uart_clear_state(puart_adapter, HAL_UART_STATE_DMARX_BUSY);

	// D-Cache sync (Invalidate)
	if (puart_adapter->dcache_invalidate_by_addr != NULL) {
		puart_adapter->dcache_invalidate_by_addr((uint32_t *)puart_adapter->prx_buf_dar, (int32_t)puart_adapter->rx_count);
	}

	puart_adapter->prx_buf = (uint8_t *)((uint32_t)(puart_adapter->prx_buf_dar) + puart_adapter->rx_count);

	// Call User Rx complete callback
	if (puart_adapter->rx_done_callback != NULL) {
		puart_adapter->rx_done_callback(puart_adapter->rx_done_cb_para);
	}
}


hal_status_t hal_uart_tx_gdma_init_ram(phal_uart_adapter_t puart_adapter, phal_gdma_adaptor_t pgdma_chnl)
{
	uint32_t dst_per = 0;

	if ((NULL == puart_adapter) || (NULL == pgdma_chnl)) {
		DBG_UART_ERR("hal_uart_tx_gdma_init: puart_adapter=0x%x pgdma_chnl=0x%x\n", puart_adapter, pgdma_chnl);
		return HAL_ERR_PARA;
	}

//    _memset((void *)pgdma_chnl, 0, sizeof(hal_gdma_adaptor_t));

	// set burst size: how many space in TX FIFO will trigger DMA req
	if (puart_adapter->tx_dma_burst_size == 0) {
		// didn't assign the TX DMA burst size, use TX FIFO trigger level to configure the TX burst size
		if (puart_adapter->base_addr->stsr_b.txfifo_low_level_status == 0) {
			puart_adapter->tx_dma_burst_size = Uart_Tx_FIFO_Size - 4;
		} else {
			puart_adapter->tx_dma_burst_size = Uart_Tx_FIFO_Size - 8;
		}
	}
	puart_adapter->base_addr->miscr_b.txdma_burstsize = puart_adapter->tx_dma_burst_size;
	puart_adapter->base_addr->fcr_b.dma_mode = 1;
	puart_adapter->base_addr->miscr_b.txdma_en = 0; // disable TX DMA at initial

	if (puart_adapter->uart_idx > Uart4) {
		DBG_UART_ERR("hal_uart_tx_gdma_init: Invalid UART Idx(%u), didn't initial? \r\n", puart_adapter->uart_idx);
		return HAL_ERR_PARA;
	}
	dst_per = uart_tx_gdma_hsk_id_tbl_ram[puart_adapter->uart_idx];

	pgdma_chnl->gdma_ctl.tt_fc      = TTFCMemToPeri;
	//pgdma_chnl->gdma_cfg.reload_dst = 1;
	pgdma_chnl->gdma_cfg.dest_per   = (u8)dst_per;
	pgdma_chnl->ch_dar = (u32) & (puart_adapter->base_addr->thr);
	pgdma_chnl->gdma_isr_type = (TransferType | ErrType);
	pgdma_chnl->gdma_ctl.int_en      = 1;
	pgdma_chnl->gdma_ctl.dinc = NoChange;
	pgdma_chnl->gdma_ctl.sinc = IncType;
	pgdma_chnl->have_chnl = 0;

	hal_gdma_handshake_init(pgdma_chnl, (uint8_t)dst_per);
	hal_gdma_irq_reg(pgdma_chnl, (irq_handler_t)_uart_tx_dma_irq_handler_ram, puart_adapter);
	puart_adapter->ptx_gdma = pgdma_chnl;
	return HAL_OK;
}

hal_status_t hal_uart_rx_gdma_init_ram(phal_uart_adapter_t puart_adapter, phal_gdma_adaptor_t pgdma_chnl)
{
	uint32_t src_per;
	uint32_t burst_size;

	if ((NULL == puart_adapter) || (NULL == pgdma_chnl)) {
		DBG_UART_ERR("hal_uart_rx_gdma_init: puart_adapter=0x%x pgdma_chnl=0x%x\n", puart_adapter, pgdma_chnl);
		return HAL_ERR_PARA;
	}

	if (puart_adapter->rx_dma_burst_size == 0) {
		// didn't assign the TX DMA burst size
		// use rx FIFO trigger level to set the DMA burst size
		switch (puart_adapter->base_addr->stsr_b.rxfifo_trigger_level_status) {
		case UartRxFifoLev_1byte:
			burst_size = 4;
			break;

		case UartRxFifoLev_8bytes:
			burst_size = 8;
			break;

		case UartRxFifoLev_16bytes:
			burst_size = 16;
			break;

		case UartRxFifoLev_28bytes:
			burst_size = 28;
			break;
		}
		puart_adapter->rx_dma_burst_size = (uint8_t)burst_size;
	}
	// how many bytes in RX FIFO will trigger DMA to read RX FIFO
	puart_adapter->base_addr->miscr_b.rxdma_burstsize = puart_adapter->rx_dma_burst_size;
	puart_adapter->base_addr->fcr_b.dma_mode = 1;
	puart_adapter->base_addr->miscr_b.rxdma_en = 0; // disable RX DMA at initial

	if (puart_adapter->uart_idx > Uart4) {
		DBG_UART_ERR("hal_uart_rx_gdma_init: Invalid UART Idx(%u), didn't initial? \r\n", puart_adapter->uart_idx);
		return HAL_ERR_PARA;
	}
	src_per = uart_rx_gdma_hsk_id_tbl_ram[puart_adapter->uart_idx];

	pgdma_chnl->gdma_ctl.tt_fc = TTFCPeriToMem;
	//pgdma_chnl->gdma_cfg.reload_src = 1;
	pgdma_chnl->gdma_cfg.src_per = (u8)src_per;
	pgdma_chnl->ch_sar = (u32) & (puart_adapter->base_addr->rbr);
	pgdma_chnl->gdma_isr_type = (TransferType | ErrType);
	pgdma_chnl->gdma_ctl.int_en = 1;
	pgdma_chnl->gdma_ctl.dinc = IncType;
	pgdma_chnl->gdma_ctl.sinc = NoChange;
	pgdma_chnl->have_chnl = 0;

	hal_gdma_handshake_init(pgdma_chnl, (uint8_t)src_per);
	hal_gdma_irq_reg(pgdma_chnl, (irq_handler_t)_uart_rx_dma_irq_handler_ram, puart_adapter);
	puart_adapter->prx_gdma = pgdma_chnl;

	return HAL_OK;
}
#endif


#if defined(CONFIG_BUILD_NONSECURE)
#if !IS_AFTER_CUT_B(CONFIG_CHIP_VER) || IS_CUT_B(CONFIG_CHIP_VER)
/**
  * @brief Over sampling table. The over sampling value for pre-defined
  *        baud rate. It is base on the UART0 SCLK = 4M.
  */
const uint8_t def_ovsr_4m_patch[] = {
	20, 17, 17, 17,
	17, 17, 13, 12,
	13, 17, 13, 17,
	13, 17, 10, 13,
	17,  5,  8,  8,
	4,  4,  2,  2,
	2,  2,  2,  1,
	1,  1,  1,  1,
	1,  1,  0
};

/**
  * @brief Divisor table. The divisor value for pre-defined
  *        baud rate. It is base on the UART0 SCLK = 4M.
  */
const uint16_t def_div_4m_patch[] = {
	1818,    784,    392,    196,
	98,     49,     32,     23,
	16,      8,      8,      4,
	4,      2,      3,      2,
	1,      2,      1,      1,
	1,      1,      1,      1,
	1,      1,      1,      1,
	1,      1,      1,      1,
	1,      1,      1
};

/**
  * @brief Bit adjustment table index for 10-bits frame.
  *             It is base on the UART0 SCLK = 4M.
  */
const uint8_t def_ovsr_adj_bit_10b_4m_patch[] = {
	0,   0,   0,   0,
	0,   0,   0,   1,
	0,   4,   0,   4,
	0,   4,   4,   0,
	4,   2,   7,   0,
	3,   0,   9,   8,
	7,   2,   0,   9,
	4,   3,   2,   1,
	1,   0,   7
};

/**
  * @brief Bit adjustment table index for 9-bits frame.
  *             It is base on the UART0 SCLK = 4M.
  */
const uint8_t def_ovsr_adj_bit_9b_4m_patch[] = {
	0,   0,   0,   0,
	0,   0,   0,   1,
	0,   3,   0,   3,
	0,   3,   4,   0,
	3,   2,   6,   0,
	3,   0,   8,   7,
	6,   1,   0,   8,
	4,   3,   2,   1,
	1,   0,   6
};

/**
  * @brief Bit adjustment table index for 8-bits frame.
  *             It is base on the UART0 SCLK = 4M.
  */
const uint8_t def_ovsr_adj_bit_8b_4m_patch[] = {
	0,   0,   0,   0,
	0,   0,   0,   1,
	0,   3,   0,   3,
	0,   3,   3,   0,
	3,   2,   5,   0,
	3,   0,   7,   6,
	5,   1,   0,   7,
	3,   3,   2,   1,
	0,   0,   5
};

/**
  * @brief Over sampling table. The over sampling value for pre-defined
  *        baud rate. It is base on the UART SCLK = 26M.
  */
const uint8_t def_ovsr_26m_patch[] = {
	20, 20, 20, 20,
	19, 19, 11, 19,
	11, 11, 13, 11,
	13, 15, 10, 13,
	14, 17, 14, 13,
	14, 13,  9, 18,
	17,  7, 13,  6,
	9,  8,  8,  7,
	6,  6,  4
};

/**
  * @brief Divisor table. The divisor value for pre-defined
  *        baud rate. It is base on the UART SCLK = 26M.
  */
const uint16_t def_div_26m_patch[] = {
	11813,  4332,  2166,  1083,
	570,   285,   246,    95,
	123,    82,    52,    41,
	26,    15,    20,    13,
	8,     4,     4,     4,
	2,     2,     2,     1,
	1,     2,     1,     2,
	1,     1,     1,     1,
	1,     1,     1
};

/**
  * @brief Bit adjustment table index for 10-bits frame.
  *             It is base on the UART0 SCLK = 26M.
  */
const uint8_t def_ovsr_adj_bit_10b_26m_patch[] = {
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   1,   0,   0,
	0,   0,   0,   2,
	1,   0,   3,   7,
	0,   3,   3,   4,
	3,   2,   3
};

/**
  * @brief Bit adjustment table index for 9-bits frame.
  *             It is base on the UART0 SCLK = 26M.
  */
const uint8_t def_ovsr_adj_bit_9b_26m_patch[] = {
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   1,   0,   0,
	0,   0,   0,   3,
	1,   0,   3,   8,
	0,   3,   3,   5,
	3,   2,   3
};

/**
  * @brief Bit adjustment table index for 8-bits frame.
  *             It is base on the UART0 SCLK = 26M.
  */
const uint8_t def_ovsr_adj_bit_8b_26m_patch[] = {
	0,  0,  0,  0,
	0,  0,  0,  0,
	0,  0,  0,  0,
	0,  1,  0,  0,
	2,  0,  3,  0,
	4,  0,  4,  2,
	3,  2,  0,  0,
	2,  3,  1,  3,
	3,  0,  5
};


/**
  * @brief Over sampling table. The over sampling value for pre-defined
  *        baud rate. It is base on the UART0 SCLK = 40M.
  */
const uint8_t def_ovsr_40m_patch[] = {
	20, 20, 20, 20,
	20, 17, 17, 15,
	10, 11, 10, 11,
	10, 15, 12, 10,
	10, 15, 17, 20,
	14, 20, 14,  9,
	13,  7, 20, 19,
	7, 13,  6,  5,
	5, 10,  6
};

/**
  * @brief Divisor table. The divisor value for pre-defined
  *        baud rate. It is base on the UART0 SCLK = 40M.
  */
const uint16_t def_div_40m_patch[] = {
	18173,  6664,  3332,  1666,
	833,   490,   245,   185,
	208,   126,   104,    63,
	52,    23,    26,    26,
	17,     7,     5,     4,
	3,     2,     2,     3,
	2,     3,     1,     1,
	2,     1,     2,     2,
	2,     1,     1
};

/**
  * @brief Bit adjustment table index for 10-bits frame.
  *             It is base on the UART0 SCLK = 40M
  */
const uint8_t def_ovsr_adj_bit_10b_40m_patch[] = {
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   1,   0,   0,
	2,   0,   4,   0,
	5,   0,   5,   2,
	3,   2,   0,   0,
	2,   3,   1,   4,
	3,   0,   7
};

/**
  * @brief Bit adjustment table index for 9-bits frame.
  *             It is base on the UART0 SCLK = 40M
  */
const uint8_t def_ovsr_adj_bit_9b_40m_patch[] = {
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   0,   0,   0,
	0,   1,   0,   0,
	2,   0,   3,   0,
	4,   0,   4,   2,
	3,   2,   0,   0,
	2,   3,   1,   4,
	3,   0,   6
};

/**
  * @brief Bit adjustment table index for 8-bits frame.
  *             It is base on the UART0 SCLK = 40M
  */
const uint8_t def_ovsr_adj_bit_8b_40m_patch[] = {
	0,  0,  0,  0,
	0,  0,  0,  0,
	0,  0,  0,  0,
	0,  1,  0,  0,
	2,  0,  3,  0,
	4,  0,  4,  2,
	3,  2,  0,  0,
	2,  3,  1,  3,
	3,  0,  5
};


hal_status_t hal_uart_lp_sclk_select_patch(phal_uart_adapter_t puart_adapter, uint8_t sclk_sel)
{
	/* NS flow */
	hal_status_t ret = HAL_OK;
	AON_TypeDef *aon_obj = AON;

	if (Uart0 != puart_adapter->uart_idx) {
		DBG_UART_ERR("%s: UART%d can not select SCLK\r\n", __func__, puart_adapter->uart_idx);
		return HAL_ERR_PARA;
	}

	/* Disable func/bus/pclk/sclk */
	hal_uart_en_ctrl(puart_adapter->uart_idx, OFF);

	switch (sclk_sel) {
	case UART_IRC_4M:
		puart_adapter->pdef_ovsr_tbl = def_ovsr_4m_patch;
		puart_adapter->pdef_div_tbl = def_div_4m_patch;
		puart_adapter->pdef_ovsradjbit_tbl10 = def_ovsr_adj_bit_10b_4m_patch;
		puart_adapter->pdef_ovsradjbit_tbl9 = def_ovsr_adj_bit_9b_4m_patch;
		puart_adapter->pdef_ovsradjbit_tbl8 = def_ovsr_adj_bit_8b_4m_patch;
		puart_adapter->uart_sclk = UartSCLK_4M;
		hal_sys_set_clk(UART0_SYS, UART_IRC_4M);
		break;

	case UART_XTAL:
		if (0x0 == aon_obj->AON_REG_AON_OTP_SYSCFG5) {
			//Xtal = 40MHz
			puart_adapter->pdef_ovsr_tbl = def_ovsr_40m_patch;
			puart_adapter->pdef_div_tbl = def_div_40m_patch;
			puart_adapter->pdef_ovsradjbit_tbl10 = def_ovsr_adj_bit_10b_40m_patch;
			puart_adapter->pdef_ovsradjbit_tbl9 = def_ovsr_adj_bit_9b_40m_patch;
			puart_adapter->pdef_ovsradjbit_tbl8 = def_ovsr_adj_bit_8b_40m_patch;
			puart_adapter->uart_sclk = UartSCLK_40M;
		} else if (0x5 == aon_obj->AON_REG_AON_OTP_SYSCFG5) {
			//Xtal = 26MHz
			puart_adapter->pdef_ovsr_tbl = def_ovsr_26m_patch;
			puart_adapter->pdef_div_tbl = def_div_26m_patch;
			puart_adapter->pdef_ovsradjbit_tbl10 = def_ovsr_adj_bit_10b_26m_patch;
			puart_adapter->pdef_ovsradjbit_tbl9 = def_ovsr_adj_bit_9b_26m_patch;
			puart_adapter->pdef_ovsradjbit_tbl8 = def_ovsr_adj_bit_8b_26m_patch;
			puart_adapter->uart_sclk = UartSCLK_26M;
		}
		hal_sys_set_clk(UART0_SYS, UART_XTAL);
		break;

	case UART_PERI_40M:
	default:
		puart_adapter->pdef_ovsr_tbl = def_ovsr_40m_patch;
		puart_adapter->pdef_div_tbl = def_div_40m_patch;
		puart_adapter->pdef_ovsradjbit_tbl10 = def_ovsr_adj_bit_10b_40m_patch;
		puart_adapter->pdef_ovsradjbit_tbl9 = def_ovsr_adj_bit_9b_40m_patch;
		puart_adapter->pdef_ovsradjbit_tbl8 = def_ovsr_adj_bit_8b_40m_patch;
		puart_adapter->uart_sclk = UartSCLK_40M;
		hal_sys_set_clk(UART0_SYS, UART_PERI_40M);
		break;
	}

	/* Enable func/bus/pclk/sclk */
	hal_uart_en_ctrl(puart_adapter->uart_idx, ON);

	/*  Set UART register default state*/
	ret = hal_uart_stubs.hal_uart_load_default_state(puart_adapter, NULL);

	DBG_UART_INFO("%s: UART%d SCLK= %d\r\n", __func__, puart_adapter->uart_idx, puart_adapter->uart_sclk);
}
#endif  // end of "#if !IS_AFTER_CUT_B(CONFIG_CHIP_VER) | IS_CUT_B(CONFIG_CHIP_VER)"

#endif  // end of "#if defined(CONFIG_BUILD_NONSECURE)"

#endif  // end of "#if CONFIG_UART_EN"

