/**************************************************************************//**
 * @file     bootloader.h
 * @brief    Declare functions for boot loader.
 *
 * @version  V1.00
 * @date     2021-11-15
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

#ifndef _BOOT_LOADER_H_
#define _BOOT_LOADER_H_
#if defined(CONFIG_BUILD_BOOT) && (CONFIG_BUILD_BOOT == 1)

#include "fw_img_tlv.h"
#include "hal_crypto.h"
#include "fw_voe_rom_boot.h"

#define RAM_FUN_TABLE_VALID_START_ADDR         (0x20100000)
#define RAM_FUN_TABLE_VALID_END_ADDR           (0x20180000)

// NOR Flash Boot
typedef int (*img_hsh_init_f_t)(const uint8_t *key, const uint32_t key_len);
typedef int (*img_hsh_update_f_t)(const uint8_t *msg, const uint32_t msg_len);
typedef int (*img_hsh_final_f_t)(uint8_t *digest);
typedef void (*flash_cpy_t)(void *, const void *, size_t);

extern boot_init_flags_t boot_init_flags;
extern sec_boot_info_t sb_ram_info;
extern uint8_t sb_digest_buf[IMG_HASH_CHK_DIGEST_MAX_SIZE];
extern hal_flash_sec_adapter_t sec_adtr;
extern hal_xip_sce_cfg_t xip_sce_cfg_pending __ALIGNED(32);
extern hal_sec_region_cfg_t sb_ram_sec_cfg_pending __ALIGNED(32);
extern uint8_t sec_digest_buf[2][20];
extern uint32_t fw1_xip_img_phy_offset; // simu
extern uint8_t tmp_img_hdr[FW_IMG_HDR_MAX_SIZE];
extern uint8_t tmp_sect_hdr[FW_IMG_HDR_MAX_SIZE];
extern uint8_t aes_gcm_tagfor_4k[512];
extern uint8_t aes_iv_info[2][4];
extern sb_crypto_enc_info_t crypto_enc_info;
extern uint8_t aes_key_info[SB_SEC_KEY_SIZE];
extern uint8_t mani_data[MANIFEST_IEDATA_SIZE];
extern fw_img_tlv_export_info_type_t fw_image_info;
extern int __voe_code_start__[];            // VOE DDR address
extern isp_multi_fcs_ld_info_t isp_multi_sensor_ld_info;

int fw_load_img_sect_f(const uint8_t *img_offset, fw_img_hdr_t *pfw_hdr, sec_boot_info_t *p_sb_info);
int sb_ram_img_vrf_op(const uint8_t sbl_cfg, sec_boot_info_t *p_sb_info, const uint8_t info_type);
uint8_t sb_ram_sect_ld_hash_prechk_valid_f(sec_boot_info_t *p_sb_info);
int sb_ram_sect_ld_hash_chk_f(uint8_t *p_msg, uint32_t msglen, uint8_t *p_img_digest_chk, sec_boot_info_t *p_sb_info);
int sec_ram_cfg_f_init(hal_sec_region_cfg_t *psb_sec_cfg_pending);
int flash_sec_crypto_ram_init(uint8_t sec_mode, uint8_t *pIv_info, uint32_t iv_len, uint8_t key_idx, uint32_t key_len);
int flash_sec_crypto_ram_decrypt(uint32_t flash_addr, uint8_t *pMsg, uint32_t msglen, uint8_t *pResult, uint8_t *pflh_Tag);
uint8_t sb_ram_sect_ld_hash_prechk_valid_f(sec_boot_info_t *p_sb_info);
uint8_t chk_img_type_id_invalid(const uint16_t type_id);
int sec_ram_key_register_key_stg(sec_boot_info_t *p_sb_info);
int confirm_manif_label_f(const uint8_t *ptr);
void record_fw_ld_info(fw_img_tlv_export_info_type_t *pfw_img_info, void *fw1_addr, void *fw2_addr, img_manifest_ld_sel_t *pld_sel_info);
int verify_fw_manif_f(const uint8_t *img_offset, const uint8_t info_type, sec_boot_info_t *p_sb_info);
int multi_sensor_info_load(const uint8_t *img_offset, isp_multi_fcs_ld_info_t *p_isp_sensor_ld_info, flash_cpy_t load_op_f);
void dump_mutli_sensor_info(isp_multi_fcs_ld_info_t *p_isp_sensor_ld_info, flash_cpy_t load_op_f);

// NAND Flash Boot
int sb_snand_hash_update(img_hsh_update_f_t img_hsh_update_f, uint32_t msglen, uint8_t *p_msg, uint8_t is_flash_dat);

__STATIC_FORCEINLINE
void ie_safe_load(void *dst, const void *src, const uint32_t size, const uint32_t max_size)
{
	uint32_t load_size;
	if (size > max_size) {
		load_size = max_size;
	} else {
		load_size = size;
	}
	memcpy(dst, src, load_size);
}

#endif  // end of CONFIG_BUILD_BOOT
#endif  // end of "#define _BOOT_LOADER_H_"
