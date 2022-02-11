/**************************************************************************//**
 * @file    hal_sdhost_nsc.h
 * @brief   The NSC interface of HAL API for SD Host controller
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
#ifndef _HAL_SDHOST_NSC_H_
#define _HAL_SDHOST_NSC_H_

#include "cmsis.h"
#include "hal_sdhost.h"

#if defined(CONFIG_BUILD_NONSECURE) || defined(CONFIG_BUILD_SECURE)
void NS_ENTRY hal_sdhost_en_ctrl_nsc(BOOL en);
void NS_ENTRY hal_sdhost_ldo_ctrl_nsc(u8 vol_sel);
void NS_ENTRY hal_sdhost_phase_shift_nsc(u8 vp_sel, u8 shift_sel);
#endif // end of !defined(CONFIG_BUILD_NONSECURE)

#endif  // end of "#define _HAL_SDHOST_NSC_H_"
