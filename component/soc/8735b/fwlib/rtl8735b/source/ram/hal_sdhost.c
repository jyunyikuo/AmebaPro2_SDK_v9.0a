/**************************************************************************//**
 * @file    hal_sdhost.c
 * @brief    This file implements NSC entries of the SD Host HAL.
 * @version V1.00
 * @date    2021-10-21
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
#include "hal_sdhost.h"
#include "hal_sdhost_nsc.h"

#if CONFIG_SDHOST_EN

hal_status_t hal_sdhost_init_host(hal_sdhost_adapter_t *psdhost_adapter)
{
	hal_status_t ret;
#if defined(CONFIG_BUILD_NONSECURE)
	ret = hal_sdhost_stubs.hal_sdhost_init_host(psdhost_adapter, NULL);
#else
	ret = hal_sdhost_stubs.hal_sdhost_init_host(psdhost_adapter, NULL);
#endif
	//psdhost_adapter->verbose = SdHostVerCmd;
	return ret;
}

hal_status_t hal_sdhost_get_csd(hal_sdhost_adapter_t *adpt, hal_sdhost_csd_t *csd)
{
	hal_status_t ret;

	ret = hal_sdhost_stubs.hal_sdhost_card_select(adpt, DISABLE);
	if (ret != HAL_OK) {
		return ret;
	}

	// CMD9 only accept [stby] state
	ret = hal_sdhost_stubs.hal_sdhost_get_csd(adpt, csd);
	if (ret != HAL_OK) {
		return ret;
	}

	ret = hal_sdhost_stubs.hal_sdhost_card_select(adpt, ENABLE);
	if (ret != HAL_OK) {
		return ret;
	}

	return HAL_OK;
}

hal_status_t hal_sdhost_get_cid(hal_sdhost_adapter_t *adpt, hal_sdhost_cid_t *cid)
{
	if (NULL == adpt || NULL == cid) {
		return HAL_ERR_PARA;
	}
	memcpy(cid, adpt->cid, SD_CID_LEN);
	return HAL_OK;
}

#endif /* CONFIG_SDHOST_EN */
