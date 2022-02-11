/**************************************************************************//**
 * @file     hal_snand.c
 * @brief    Functions to implement the Serial NAND Flash Controller (SNAFC) operation.
 * @version  1.00
 * @date     2020-12-12
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2020 Realtek Corporation. All rights reserved.
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
#include "hal_snand.h"
#include "hal_pinmux.h"
#include "hal_cache.h"
#if CONFIG_FPGA
#include "hal_gpio.h"
#endif

extern const hal_snand_func_stubs_t hal_snand_stubs;

/**

        \addtogroup hal_snand Flash Controller
        @{
*/

/**
 ++ S-NAND flash dependent callback function implementation ++
*/
static uint32_t mSnandWaitRdyFuncType0(uint32_t timeout);
static uint32_t mSnandChkWelStsFuncType0(void);
static uint32_t mSnandChkEFaillStsFuncType0(void);
static uint32_t mSnandChkPFailStsFuncType0(void);
static uint32_t mSnandChkEccStsFuncType0(void);

static uint32_t mSnandEnableQuadBusModeType0(void);
static uint32_t mSnandDisableQuadBusModeType0(void);
static uint32_t mSnandEnableOnChipEccType0(void);
static uint32_t mSnandDisableOnChipEccType0(void);

static uint32_t mSnandBlockProtectNoneFuncType0(void);
static uint32_t mSnandBlockProtectAllFuncType0(void);
static uint32_t mSnandBlockProtectNoneFuncType1(void);
static uint32_t mSnandBlockProtectAllFuncType1(void);

static uint32_t
mSnandWaitRdyFuncType0(
	uint32_t timeout
)
{
	uint32_t stsReg = 0xc0;
	uint32_t retVal;
	uint32_t swTo = 0;
#define WAIT_OIP_RDY_MSK (0x100)
#define WAIT_OIP_RDY_MAX (0x800)
	retVal = hal_snand_get_status(NULL, stsReg);
	while ((retVal >> 0 & 0x1) != 0) {
		retVal = hal_snand_get_status(NULL, stsReg);
		swTo++;
		if (swTo > WAIT_OIP_RDY_MAX) {
			break;
		}
	}
	if (swTo > WAIT_OIP_RDY_MAX) {
		return FAIL;
	}
	return SUCCESS;
}

static uint32_t
mSnandChkWelStsFuncType0(
	void
)
{
	uint32_t stsReg = 0xc0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	if (retVal >> 1 & 0x1) {
		return SNAND_STS_WRITE_ENABLE_LATCHED;
	}
	return SNAND_STS_WEL_NONE;
}

static uint32_t
mSnandChkEFaillStsFuncType0(
	void
)
{
	uint32_t stsReg = 0xc0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	if (retVal >> 2 & 0x1) {
		return SNAND_STS_ERASE_FAIL;
	}
	return SNAND_STS_ERASE_NO_ERR;
}

static uint32_t
mSnandChkPFailStsFuncType0
(
	void
)
{
	uint32_t stsReg = 0xc0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	if (retVal >> 3 & 0x1) {
		return SNAND_STS_PROGRAM_FAIL;
	}
	return SNAND_STS_PROGRAM_NO_ERR;
}

static uint32_t
mSnandChkEccStsFuncType0(
	void
)
{
	uint32_t stsReg = 0xc0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	switch (retVal >> 4 & 0x3) {
	case 0:
		return SNAND_STS_ECC_NO_ERR;
	case 1:
		return SNAND_STS_ECC_ERR_AND_FIXED;
	default:
	case 2:
		return SNAND_STS_ECC_ERR_CANNOT_FIX;
	}
}

static uint32_t
mSnandEnableQuadBusModeType0(
	void
)
{
	uint32_t stsReg = 0xb0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal |= 0x01;
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

static uint32_t
mSnandDisableQuadBusModeType0(
	void
)
{
	uint32_t stsReg = 0xb0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal &= ~0x01;
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

static uint32_t
mSnandEnableOnChipEccType0(
	void
)
{
	uint32_t stsReg = 0xb0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal |= (0x01 << 4);
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

static uint32_t
mSnandDisableOnChipEccType0(
	void
)
{
	uint32_t stsReg = 0xb0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal &= ~(0x01 << 4);
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

static uint32_t
mSnandBlockProtectNoneFuncType0(
	void
)
{
	uint32_t stsReg = 0xa0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal &= ~(0x07 << 3);
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

static uint32_t
mSnandBlockProtectAllFuncType0(
	void
)
{
	uint32_t stsReg = 0xa0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal |= (0x07 << 3);
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

static uint32_t
mSnandBlockProtectNoneFuncType1(
	void
)
{
	uint32_t stsReg = 0xa0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal &= ~(0x0F << 3);
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

static uint32_t
mSnandBlockProtectAllFuncType1(
	void
)
{
	uint32_t stsReg = 0xa0;
	uint32_t retVal;
	retVal = hal_snand_get_status(NULL, stsReg);
	retVal |= (0x0F << 3);
	hal_snand_set_status(NULL, stsReg, retVal);
	return SUCCESS;
}

/**
 -- S-NAND flash dependent callback function implementation ++
*/

/**
 ++ GLOBAL-TOP-dependent information (not specified by SNAFC) ++
*/
hal_status_t hal_snand_pinmux_ctl(hal_snafc_adaptor_t *pAdaptor /* unused */, uint8_t ctl)
{
	hal_status_t ret = HAL_OK;
#if CONFIG_FPGA /* Philip@2021/06/03, DV's Saurabh Adhikari said, during SNAFC enable, C1/C3 should not set to pull-up */
	uint8_t quad_pin_sel = 0; /* 0 for 1-pin, 2-pin, !0 for 4-pin */
#endif

	if (ctl == ENABLE) {
#if CONFIG_FPGA
		hal_pinmux_register(PIN_C0, PID_GPIO);
		hal_pinmux_register(PIN_C2, PID_GPIO);
		hal_pinmux_register(PIN_C4, PID_GPIO);
		hal_pinmux_register(PIN_C5, PID_GPIO);

		if (quad_pin_sel) {
			hal_pinmux_register(PIN_C1, PID_GPIO);
			hal_pinmux_register(PIN_C3, PID_GPIO);
		} else {
			hal_gpio_pull_ctrl((u32)PIN_C1, Pin_PullUp);
			hal_gpio_pull_ctrl((u32)PIN_C3, Pin_PullUp);
		}
#else
		hal_pinmux_register(PIN_C0, PID_FLASH);
		hal_pinmux_register(PIN_C2, PID_FLASH);
		hal_pinmux_register(PIN_C4, PID_FLASH);
		hal_pinmux_register(PIN_C5, PID_FLASH);

#if 1 /* Philip@2021/06/03, DV's Saurabh Adhikari said, during SNAFC enable, C1/C3 should not set to pull-up */
		hal_pinmux_register(PIN_C1, PID_FLASH);
		hal_pinmux_register(PIN_C3, PID_FLASH);
#else /* original */
		if (quad_pin_sel) {
			hal_pinmux_register(PIN_C1, PID_FLASH);
			hal_pinmux_register(PIN_C3, PID_FLASH);
		} else {
			hal_gpio_pull_ctrl((u32)PIN_C1, Pin_PullUp);
			hal_gpio_pull_ctrl((u32)PIN_C3, Pin_PullUp);
		}
#endif
#endif
	} else {
#if CONFIG_FPGA
		hal_pinmux_unregister(PIN_C0, PID_GPIO);
		hal_pinmux_unregister(PIN_C2, PID_GPIO);
		hal_pinmux_unregister(PIN_C4, PID_GPIO);
		hal_pinmux_unregister(PIN_C5, PID_GPIO);

		if (quad_pin_sel) {
			hal_pinmux_unregister(PIN_C1, PID_GPIO);
			hal_pinmux_unregister(PIN_C3, PID_GPIO);
		} else {
			hal_gpio_pull_ctrl((u32)PIN_C1, Pin_PullDefault);
			hal_gpio_pull_ctrl((u32)PIN_C3, Pin_PullDefault);
		}
#else
		hal_pinmux_unregister(PIN_C0, PID_FLASH);
		hal_pinmux_unregister(PIN_C2, PID_FLASH);
		hal_pinmux_unregister(PIN_C4, PID_FLASH);
		hal_pinmux_unregister(PIN_C5, PID_FLASH);

#if 1 /* Philip@2021/06/03, DV's Saurabh Adhikari said, during SNAFC enable, C1/C3 should not set to pull-up */
		hal_pinmux_unregister(PIN_C1, PID_FLASH);
		hal_pinmux_unregister(PIN_C3, PID_FLASH);
#else /* original */
		if (quad_pin_sel) {
			hal_pinmux_unregister(PIN_C1, PID_FLASH);
			hal_pinmux_unregister(PIN_C3, PID_FLASH);
		} else {
			hal_gpio_pull_ctrl((u32)PIN_C1, Pin_PullDefault);
			hal_gpio_pull_ctrl((u32)PIN_C3, Pin_PullDefault);
		}
#endif
#endif
	}

	return ret;
}/* hal_snand_pinmux_ctl */
/**
 -- GLOBAL-TOP-dependent information (not specified by SNAFC) --
*/

/**

        \addtogroup hs_hal_snand_ram_func Flash Controller HAL RAM APIs
        \ingroup hs_hal_snand
        \brief The flash controller HAL APIs. Functions become an interface between API functions and ROM codes.
        @{
*/

void
hal_snand_init(
	hal_snafc_adaptor_t *pAdaptor
)
{
	hal_snand_pinmux_ctl(pAdaptor, ENABLE);
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	hal_rtl_snand_init(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	hal_snand_stubs.hal_snand_init(pAdaptor);
#endif
	return;
} /* hal_snand_init */


void
hal_snand_deinit(
	hal_snafc_adaptor_t *pAdaptor
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	hal_rtl_snand_deinit(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	hal_snand_stubs.hal_snand_deinit(pAdaptor);
#endif
	hal_snand_pinmux_ctl(pAdaptor, DISABLE);
	return;
} /* hal_snand_deinit */


uint32_t
hal_snand_reset_to_spi(
	hal_snafc_adaptor_t *pAdaptor
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_issusResetOpCmd(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_reset_to_spi(pAdaptor);
#endif
} /* hal_snand_reset_to_spi */


uint32_t
hal_snand_read_id(
	hal_snafc_adaptor_t *pAdaptor
)
{
	uint32_t retVal;
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_issueReadIdOpCmd(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	retVal = hal_snand_stubs.hal_snand_read_id(pAdaptor);
#endif
	if (pAdaptor) {
		pAdaptor->devId[0] = (retVal >> 16) & 0xFF;
		pAdaptor->devId[1] = (retVal >> 8) & 0xFF;
		pAdaptor->devId[2] = (retVal >> 0) & 0xFF;
	}
	return retVal;
} /* hal_snand_read_id */


void
hal_snand_set_quad_enable(
	hal_snafc_adaptor_t *pAdaptor
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_quadModeEnable(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_set_quad_enable(pAdaptor);
#endif
} /* hal_snand_set_quad_enable */


void
hal_snand_unset_quad_enable(
	hal_snafc_adaptor_t *pAdaptor
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_quadModeDisable(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_unset_quad_enable(pAdaptor);
#endif
} /* hal_snand_unset_quad_enable */



void
hal_snand_set_status(
	hal_snafc_adaptor_t *pAdaptor,
	uint8_t cmd,
	uint8_t data
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_issueSetFeatureRegisterOpCmd(pAdaptor, cmd, data); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_set_feature(pAdaptor, cmd, data);
#endif
} /* hal_snand_set_status */


void
hal_snand_set_status_no_check(
	hal_snafc_adaptor_t *pAdaptor,
	uint8_t cmd,
	uint8_t data
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_issueSetFeatureRegisterOpCmd_NoCheck(pAdaptor, cmd, data); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_set_feature_no_check(pAdaptor, cmd, data);
#endif
} /* hal_snand_set_status_no_check */


uint32_t
hal_snand_get_status(
	hal_snafc_adaptor_t *pAdaptor,
	uint8_t cmd
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_issueGetFeatureRegisterOpCmd(pAdaptor, cmd); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_get_feature(pAdaptor, cmd);
#endif
} /* hal_snand_get_status */


uint32_t
hal_snand_wait_ready(
	hal_snafc_adaptor_t *pAdaptor
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_waitSnandOipComplete(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_wait_ready(pAdaptor);
#endif
} /* hal_snand_wait_ready */


void
hal_snand_set_write_enable(
	hal_snafc_adaptor_t *pAdaptor
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_issueWriteEnableOpCmd(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_set_write_enable(pAdaptor);
#endif
} /* hal_snand_set_write_enable */


void
hal_snand_set_write_disable(
	hal_snafc_adaptor_t *pAdaptor
)
{
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	return hal_rtl_snand_issueWriteDisableOpCmd(pAdaptor); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	return hal_snand_stubs.hal_snand_set_write_disable(pAdaptor);
#endif
} /* hal_snand_set_write_disable */


uint32_t
hal_snand_block_erase(
	hal_snafc_adaptor_t *pAdaptor,
	uint32_t blkPageAddr
)
{
	uint32_t retVal = SUCCESS;
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_issueBlockEraseOpCmd(pAdaptor, blkPageAddr); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	retVal = hal_snand_stubs.hal_snand_block_erase(pAdaptor, blkPageAddr);
#endif
	return retVal;
} /* hal_snand_block_erase */


uint32_t
hal_snand_page_program(
	hal_snafc_adaptor_t *pAdaptor,
	uint32_t blkPageAddr
)
{
	uint32_t retVal = SUCCESS;
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_issueProgramExecuteOpCmd(pAdaptor, blkPageAddr); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	retVal = hal_snand_stubs.hal_snand_page_program(pAdaptor, blkPageAddr);
#endif
	return retVal;
} /* hal_snand_page_program */


uint32_t
hal_snand_pio_read(
	hal_snafc_adaptor_t *pAdaptor,
	void *memAddr,
	uint32_t dataLens,
	uint32_t blkPageAddr
)
{
	uint32_t retVal = SUCCESS;
	if (pAdaptor) {
		pAdaptor->dma_en = 0;
	}
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_pageRead(pAdaptor, memAddr, dataLens, blkPageAddr); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	retVal = hal_snand_stubs.hal_snand_pageRead(pAdaptor, memAddr, dataLens, blkPageAddr);
#endif
	if (pAdaptor) {
		pAdaptor->dma_en = 1;
	}
	return retVal;
} /* hal_snand_pio_read */

uint32_t
hal_snand_page_read(
	hal_snafc_adaptor_t *pAdaptor,
	void *memAddr,
	uint32_t dataLens,
	uint32_t blkPageAddr
)
{
	uint32_t retVal = SUCCESS;
	u32 backup_col_addr = pAdaptor->col_addr;
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_pageRead(pAdaptor, memAddr, dataLens, blkPageAddr); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	// Workaround dma prefix and postfix
	if ((pAdaptor != NULL) && (pAdaptor->dma_en == 1)) {
		pAdaptor->dma_en = 0;
		u8 *cur_dest = memAddr;
		u32 size_left = dataLens;
		u32 cur_page = blkPageAddr;
		u32 align_size, unalign_size;
		// Workaround dma engine limit
		// 1. start address 16 byte align
		// 2. size 4 byte align
		// 3. Extend: start address 32 byte align (for cache)
		u32 unalign_start = (u32)cur_dest & (32 - 1);

		if (unalign_start > 0) {
			unalign_size = 32 - unalign_start;
			if (unalign_size > size_left) {
				unalign_size = size_left;
			}

			// pio_read first unalign data
			pAdaptor->dma_en = 0;
			retVal = hal_snand_stubs.hal_snand_pageRead(pAdaptor, cur_dest, unalign_size, cur_page);
			if (retVal != SUCCESS) {
				return retVal;
			}

			cur_dest += unalign_size;
			pAdaptor->col_addr += unalign_size;
			size_left -= unalign_size;
		}

		if (size_left > 0) {
			pAdaptor->dma_en = 1;
			align_size = size_left & (~0x3);
			// Handle cache
			dcache_clean_by_addr((u32 *)cur_dest, align_size);

			retVal = hal_snand_stubs.hal_snand_pageRead(pAdaptor, cur_dest, align_size, cur_page);
			if (retVal != SUCCESS) {
				return retVal;
			}
			// Handle cache
			dcache_invalidate_by_addr((u32 *)cur_dest, align_size);

			cur_dest += align_size;
			pAdaptor->col_addr += align_size;
			size_left -= align_size;
		}

		if (size_left > 0) {
			// Handle last unalign 4-byte data
			pAdaptor->dma_en = 0;
			retVal = hal_snand_stubs.hal_snand_pageRead(pAdaptor, cur_dest, size_left, cur_page);
			if (retVal != SUCCESS) {
				return retVal;
			}
		}
		pAdaptor->dma_en = 1;
	} else {
		retVal = hal_snand_stubs.hal_snand_pageRead(pAdaptor, memAddr, dataLens, blkPageAddr);
	}
#endif
	pAdaptor->col_addr = backup_col_addr;
	return retVal;
} /* hal_snand_pio_read */

uint32_t
hal_snand_pio_write(
	hal_snafc_adaptor_t *pAdaptor,
	void *memAddr,
	uint32_t dataLens,
	uint32_t blkPageAddr
)
{
	uint32_t retVal = SUCCESS;
	if (pAdaptor) {
		pAdaptor->dma_en = 0;
	}
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_pageWrite(pAdaptor, memAddr, dataLens, blkPageAddr); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	retVal = hal_snand_stubs.hal_snand_pageWrite(pAdaptor, memAddr, dataLens, blkPageAddr);
#endif
	if (pAdaptor) {
		pAdaptor->dma_en = 1;
	}
	return retVal;
} /* hal_snand_pio_write */


uint32_t
hal_snand_dma_read(
	hal_snafc_adaptor_t *pAdaptor,
	void *memAddr,
	uint32_t dataLens,
	uint32_t blkPageAddr
)
{
	uint32_t retVal = SUCCESS;
	if (pAdaptor) {
		pAdaptor->dma_en = 1;
	}
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_pageRead(pAdaptor, memAddr, dataLens, blkPageAddr); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	retVal = hal_snand_stubs.hal_snand_pageRead(pAdaptor, memAddr, dataLens, blkPageAddr);
#endif
	return retVal;
} /* hal_snand_dma_read */


uint32_t
hal_snand_dma_write(
	hal_snafc_adaptor_t *pAdaptor,
	void *memAddr,
	uint32_t dataLens,
	uint32_t blkPageAddr
)
{
	uint32_t retVal = SUCCESS;
	if (pAdaptor) {
		pAdaptor->dma_en = 1;
	}
#if IS_CUT_TEST(CONFIG_CHIP_VER)
	retVal = hal_rtl_snand_pageWrite(pAdaptor, memAddr, dataLens, blkPageAddr); /* ram_s/rtl8735b_snand.c instead of rom_s/rtl8735b_snand.c */
#else
	retVal = hal_snand_stubs.hal_snand_pageWrite(pAdaptor, memAddr, dataLens, blkPageAddr);
#endif

	return retVal;
} /* hal_snand_dma_write */

/** *@} */ /* End of group hal_snand_ram_func */

/** *@} */ /* End of group hal_snand */

