/**************************************************************************//**
 * @file     hal_trng.c
 * @brief    This TRNG HAL API functions.
 *
 * @version  V1.00
 * @date     2021-03-8
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

#include "hal_trng.h"
#include "hal_sys_ctrl.h"
#include "hal_sys_ctrl.h"


#if defined(CONFIG_BUILD_NONSECURE)
#include "hal_sys_ctrl_nsc.h"
#endif

#if CONFIG_TRNG_EN
/**
 * @addtogroup ls_hal_trng TRNG
 * @{
 */


/**
 *  @brief To enable a TRNG devive.
 *
 *  @param[in] ptrng_adp The TRNG device adapter.
 *
 *  @returns void
 */
void hal_trng_enable_128K(hal_trng_adapter_t *ptrng_adp)
{
	//dbg_printf("hal_trng_enable_128K  \n\r ");
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
#if  defined (CONFIG_BUILD_NONSECURE)
	hal_sys_peripheral_en(TRNG_128K, ENABLE);
#endif
	hal_rtl_trng_enable_128K_patch(ptrng_adp);
#else // MP Chip A-Cut
#if  defined (CONFIG_BUILD_NONSECURE)
	hal_sys_peripheral_en(TRNG_128K, ENABLE);
#endif
	hal_trng_stubs.hal_trng_enable_128K(ptrng_adp);
#endif
	hal_delay_us(1);
}
/**
 *  @brief To enable a TRNG devive and input a wave form.
 *
 *  @param[in] ptrng_adp The TRNG device adapter.
 *
 *  @returns void
 */
void hal_trng_enable_32K(hal_trng_adapter_t *ptrng_adp)
{
	//dbg_printf("hal_trng_enable_32K  \n\r ");
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
#if  defined (CONFIG_BUILD_NONSECURE)
	hal_sys_peripheral_en(TRNG_32K, ENABLE);
	//dbg_printf("hal_trng 82  \n\r ");
#endif
	hal_rtl_trng_enable_32K_patch(ptrng_adp);
	//dbg_printf("hal_trng 84  \n\r ");
#else // MP Chip A-Cut
#if  defined (CONFIG_BUILD_NONSECURE)
	hal_sys_peripheral_en(TRNG_32K, ENABLE);
#endif
	hal_trng_stubs.hal_trng_enable_32K(ptrng_adp);
#endif
	hal_delay_us(1);

}

/**
 *  @brief To disable a TRNG devive.
 *
 *  @param[in] ptrng_adp The TRNG device adapter.
 *
 *  @returns void
 */
void hal_trng_disable(hal_trng_adapter_t *ptrng_adp)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
#if  defined (CONFIG_BUILD_NONSECURE)
	hal_sys_peripheral_en(TRNG_SYS, DISABLE);
#endif
	hal_rtl_trng_disable_patch(ptrng_adp);
#else // MP Chip A-Cut
#if  defined (CONFIG_BUILD_NONSECURE)
	hal_sys_peripheral_en(TRNG_SYS, DISABLE);
#endif
	hal_trng_stubs.hal_trng_disable(ptrng_adp);
#endif
}


/**
 *  @brief To disable a TRNG devive.
 *
 *  @param[in] ptrng_adp The TRNG device adapter.
 *
 *  @returns void
 */
void hal_trng_reg_irq(irq_handler_t handler, uint32_t *arg)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	hal_rtl_trng_reg_irq_patch(handler, arg);
#else // MP Chip A-Cut
	hal_trng_stubs.hal_trng_reg_irq(handler, arg);
#endif
}

/**
 *  \brief To set the control of the TRNG
 *
 *  \param[in] ptrng_adp The TRNG device adapter.

 *   *  @param[in]  data
 *
 *  \returns     HAL_OK:  Setting succeed.
 *  \returns     HAL_ERR_PARA:  Input arguments are invalid.
 */

hal_status_t hal_trng_control_setting(hal_trng_adapter_t *ptrng_adp, hal_rng_reg_t *ptrng_reg)

{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	return hal_rtl_trng_control_setting_patch(ptrng_adp, ptrng_reg);
#else // MP Chip A-Cut
	return hal_trng_stubs.hal_trng_control_setting(ptrng_adp, ptrng_reg);
#endif
}
/**
 *  @brief To initial a TRNG devices adapter.
 *
 *  @param[in]  The TRNG devices adapter.
 *
 *  @returns HAL_OK: Initial succeed.
 */
hal_status_t hal_trng_init(hal_trng_adapter_t *ptrng_adp)
{
	////dbg_printf("hal_trng_init \r\n");
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	hal_rtl_trng_init_patch(ptrng_adp);
#else // MP Chip A-Cut
	hal_trng_stubs.hal_trng_init(ptrng_adp);
	return HAL_OK;
#endif

}

/**
 *  @brief To deinitial a TRNG devices adapter.
 *
 *  @param[in]  The TRNG devices adapter.
 *
 *  @returns HAL_OK: Initial succeed.
 */
hal_status_t hal_trng_deinit(hal_trng_adapter_t *ptrng_adp)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	hal_rtl_trng_deinit_patch(ptrng_adp);
#else // MP Chip A-Cut
	hal_trng_stubs.hal_trng_deinit(ptrng_adp);
#endif
	return HAL_OK;
}


/**
 *  @brief To deinitial a TRNG devices adapter.
 *
 *  @param[in]  The TRNG devices adapter.
 *
 *  @returns HAL_OK: Initial succeed.
 */
hal_status_t hal_trng_load_default(hal_trng_adapter_t *ptrng_adp, hal_rng_st_reg_t *ptrng_st_reg,  hal_rng_reg_t *ptrng_reg)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	hal_rtl_trng_load_default_patch(ptrng_adp, ptrng_st_reg, ptrng_reg);
#else // MP Chip A-Cut
	hal_trng_stubs.hal_trng_load_default(ptrng_adp, ptrng_st_reg, ptrng_reg);
#endif
	return HAL_OK;
}

/**
 *  \brief self test setting
 *
 *  \param[in] ptrng_adp The TRNG device adapter.
 *
 *  \returns     hal_status_t
 */

void hal_trng_reset(hal_trng_adapter_t *ptrng_adp)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	hal_rtl_trng_reset_patch(ptrng_adp);
#else // MP Chip A-Cut
	hal_trng_stubs.hal_trng_reset(ptrng_adp);
#endif

}


/**
 *  \brief self test setting
 *
 *  \param[in] ptrng_adp The TRNG device adapter.
 *
 *  \returns     hal_status_t
 */

void hal_trng_self_test_setting(hal_trng_adapter_t *ptrng_adp, hal_rng_st_reg_t *ptrng_st_reg)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	hal_rtl_trng_self_test_setting_patch(ptrng_adp, ptrng_st_reg);
#else // MP Chip A-Cut
	hal_trng_stubs.hal_trng_self_test_setting(ptrng_adp, ptrng_st_reg);
#endif
}

/**
 *  \brief To set the interrupt enable of the TRNG
 *
 *  \param[in] ptrng_adp The TRNG device adapter.

 *   *  @param[in]   information data
 *
 *  \returns     HAL_OK:  Setting succeed.
 *  \returns     HAL_ERR_PARA:  Input arguments are invalid.
 */
hal_status_t hal_trng_interrupt_reg(hal_trng_adapter_t *ptrng_adp, uint32_t *data)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	hal_rtl_trng_interrupt_reg_patch(ptrng_adp, data);
#else // MP Chip A-Cut
	hal_trng_stubs.hal_trng_interrupt_reg(ptrng_adp, data);
#endif
	return HAL_OK;
}


/**
 *  \brief read ready bit of the TRNG
 *
 *  \param[in] ptrng_adp The TRNG device adapter.

 *   *  @param[in]   information data
 *
 *  \returns     HAL_OK:  Setting succeed.
  *  \returns     HAL_ERR_PARA:  Input arguments are invalid.

 */

u32 hal_trng_read_readybit(hal_trng_adapter_t *ptrng_adp)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	return hal_rtl_trng_read_readybit_patch(ptrng_adp);
#else // MP Chip A-Cut
	return  hal_trng_stubs.hal_trng_read_readybit(ptrng_adp);
#endif

}


/**
 *  \brief read error interrupt of the TRNG
 *
 *  \param[in] ptrng_adp The TRNG device adapter.

 *   *  @param[in]   information data
 *
 *
 */

u32 hal_trng_read_parity_error_interrupt(hal_trng_adapter_t *ptrng_adp)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	return hal_rtl_trng_read_parity_error_interrupt_patch(ptrng_adp);
#else // MP Chip A-Cut
	return  hal_trng_stubs.hal_trng_read_parity_error_interrupt(ptrng_adp);
#endif

}


/**
 *  \brief clear interrupt register
 *
 *  \param[in] ptrng_adp The TRNG device adapter.

 *   *  @param[in]   information data
 *
 *  \returns     HAL_OK:  Setting succeed.
  *  \returns     HAL_ERR_PARA:  Input arguments are invalid.

 */

u32 hal_trng_clear_interrupt(hal_trng_adapter_t *ptrng_adp)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	return  hal_rtl_trng_clear_interrupt_patch(ptrng_adp);
#else // MP Chip A-Cut
	return  hal_trng_stubs.hal_trng_clear_interrupt(ptrng_adp);
#endif

}


/**
 *  \brief read data of the TRNG
 *
 *  \param[in] ptrng_adp The TRNG device adapter.

 *   *  @param[in]   information data
 *
 *  \returns     trng data
 */

u32 hal_trng_read_data(hal_trng_adapter_t *ptrng_adp)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER) // Test Chip
	return  hal_rtl_trng_read_data_patch(ptrng_adp);
#else // MP Chip A-Cut
	return  hal_trng_stubs.hal_trng_read_data(ptrng_adp);
#endif

}

#endif

