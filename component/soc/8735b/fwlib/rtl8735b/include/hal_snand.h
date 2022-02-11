/**************************************************************************//**
 * @file     hal_snand.h
 * @brief    The header file of hal_snand.c.
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

#ifndef _HAL_SNAND_H_
#define _HAL_SNAND_H_
#include "cmsis.h"

#ifdef  __cplusplus
extern "C"
{
#endif

/**

        \addtogroup hs_hal_snand Serial-NAND FLASH
        @{
*/

/**

        \addtogroup hs_hal_snand_ram_func Serial-NAND FLASH HAL RAM APIs
        \ingroup hs_hal_snand
        @{
*/

void hal_snand_init(hal_snafc_adaptor_t *pAdaptor);
void hal_snand_deinit(hal_snafc_adaptor_t *pAdaptor);
uint32_t hal_snand_reset_to_spi(hal_snafc_adaptor_t *pAdaptor);
uint32_t hal_snand_read_id(hal_snafc_adaptor_t *pAdaptor);
void hal_snand_set_quad_enable(hal_snafc_adaptor_t *pAdaptor);
void hal_snand_unset_quad_enable(hal_snafc_adaptor_t *pAdaptor);
void hal_snand_set_status(hal_snafc_adaptor_t *pAdaptor, uint8_t cmd, uint8_t data);
void hal_snand_set_status_no_check(hal_snafc_adaptor_t *pAdaptor, uint8_t cmd, uint8_t data);
uint32_t hal_snand_get_status(hal_snafc_adaptor_t *pAdaptor, uint8_t cmd);
uint32_t hal_snand_wait_ready(hal_snafc_adaptor_t *pAdaptor);
void hal_snand_set_write_enable(hal_snafc_adaptor_t *pAdaptor);
void hal_snand_set_write_disable(hal_snafc_adaptor_t *pAdaptor);
uint32_t hal_snand_block_erase(hal_snafc_adaptor_t *pAdaptor, uint32_t blkPageAddr);
uint32_t hal_snand_page_program(hal_snafc_adaptor_t *pAdaptor, uint32_t blkPageAddr);
uint32_t hal_snand_pio_read(hal_snafc_adaptor_t *pAdaptor, void *memAddr, uint32_t dataLens, uint32_t blkPageAddr);
uint32_t hal_snand_pio_write(hal_snafc_adaptor_t *pAdaptor, void *memAddr, uint32_t dataLens, uint32_t blkPageAddr);
uint32_t hal_snand_dma_read(hal_snafc_adaptor_t *pAdaptor, void *memAddr, uint32_t dataLens, uint32_t blkPageAddr);
uint32_t hal_snand_dma_write(hal_snafc_adaptor_t *pAdaptor, void *memAddr, uint32_t dataLens, uint32_t blkPageAddr);
uint32_t hal_snand_page_read(hal_snafc_adaptor_t *pAdaptor, void *memAddr, uint32_t dataLens, uint32_t blkPageAddr);

/** *@} */ /* End of group hs_hal_snand_ram_func */

/** *@} */ /* End of group hs_hal_snand */

#ifdef  __cplusplus
}
#endif


#endif  // end of "#define _HAL_SNAND_H_"


