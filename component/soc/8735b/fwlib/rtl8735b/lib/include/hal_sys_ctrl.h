/**************************************************************************//**
* @file        hal_sys_ctrl.h
* @brief       The HAL API implementation for the System control
*
* @version     V1.00
* @date        2021-11-24
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



#ifndef _HAL_SYS_CTRL_H_
#define _HAL_SYS_CTRL_H_
#include "cmsis.h"
#include "rtl8735b_sys_ctrl.h"
#include "fw_img_tlv.h"

#ifdef  __cplusplus
extern "C"
{
#endif

typedef enum {
	BT_UART_MUX_EXTERNAL = 0,
	BT_UART_MUX_INTERNAL = 1
} BT_UART_MUX_SELECT_t;

enum dbg_port_mode_e {
	DBG_PORT_OFF    = 0,    ///< debug port off
	SWD_MODE        = 1,    ///< debugger use SWD mode
	JTAG_MODE       = 2,    ///< debugger use JTAG mode
};
typedef uint8_t     dbg_port_mode_t;

enum dbg_port_pin_sel_e {
	TMS_IO_S0_CLK_S0   = 0,
	TMS_IO_S1_CLK_S0   = 1,
	TMS_IO_S0_CLK_S1   = 2,
	TMS_IO_S1_CLK_S1   = 3
};
typedef uint8_t     dbg_port_pin_sel_t;


/**
* @addtogroup N/A
* @{
*/

void hal_sys_get_chip_id(uint32_t *pchip_id);
void hal_sys_peripheral_en(uint8_t id, uint8_t en);
void hal_sys_lxbus_shared_en(uint8_t used_id, uint8_t en);
void hal_sys_set_clk(uint8_t id, uint8_t sel_val);
uint32_t hal_sys_get_clk(uint8_t id);
void hal_sys_set_system_reset(void);
hal_status_t hal_sys_dbg_port_cfg(dbg_port_mode_t dbg_mode, dbg_port_pin_sel_t pin_sel);
uint8_t hal_sys_get_boot_select(void);

void hal_sys_set_bus_idau(uint32_t idau_idx, uint32_t start_addr, uint32_t end_addr);

uint32_t hal_sys_boot_info_get_val(uint8_t info_idx);
void hal_sys_boot_info_assign_val(uint8_t info_idx, uint32_t info_v);
void hal_sys_boot_footpath_init(uint8_t info_idx);
void hal_sys_boot_footpath_store(uint8_t info_idx, uint8_t fp_v);
void hal_sys_boot_footpath_clear(uint8_t info_idx, uint8_t fp_v);
uint8_t hal_sys_get_rma_state(void);
void hal_sys_sjtag_fixed_key_enable(void);
void hal_sys_sjtag_non_fixed_key_set(uint8_t set_sjtag_obj, uint8_t *pkey);
uint32_t hal_sys_get_video_info(uint8_t idx);
void hal_sys_get_rom_v(uint8_t *ver_main, uint8_t *ver_sub);
uint8_t hal_sys_get_ld_fw_idx(void);

void hal_sdm_32k_enable(u8 bypass_mode);
u32 hal_read_sdm_32k_time_loss(void);
void hal_xtal_divider_enable(u8 enable);
void hal_aon_wdt_enable(u8 enable, u32 timeout);
void hal_osc4m_cal(void);
void hal_pll_98p304_ctrl(u8 en, u8 clk_src);
void hal_pll_45p158_ctrl(u8 en, u8 clk_src);
void hal_32k_s1_sel(u8 sel);
void hal_xtal_32768_en(u8 enable);

#define RAM_FOOTPH_INIT(idx)                hal_sys_boot_footpath_init(idx)
#define RAM_FOOTPH_STORE(idx,fp_v)          hal_sys_boot_footpath_store(idx,fp_v)
#define RAM_FOOTPH_CLR(idx,fp_v)            hal_sys_boot_footpath_clear(idx,fp_v)
#define RAM_FOOTPH_GET(idx)                 hal_sys_boot_info_get_val(idx)


/** @} */ /* End of group hs_hal_efuse */

#ifdef  __cplusplus
}
#endif


#endif  // end of "#define _HAL_SYS_CTRL_H_"


