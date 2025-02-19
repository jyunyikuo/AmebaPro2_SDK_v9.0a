/**************************************************************************//**
 * @file     fw_flash_boot.h
 * @brief    Declare functions for boot from nor flash.
 *
 * @version  V1.00
 * @date     2021-08-06
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

#ifndef _FW_FLASH_BOOT_H_
#define _FW_FLASH_BOOT_H_

#include "fw_img_tlv.h"

#if defined(ROM_REGION)

extern hal_crypto_adapter_t sb_rom_crypto_adtr;
extern const flash_pin_sel_t boot_flash_pins[FLASH_PINS_MAX_SEL];
extern u8 mani_data[MANIFEST_IEDATA_SIZE];
extern uint8_t tmp_img_hdr[FW_IMG_HDR_MAX_SIZE];
extern uint8_t tmp_sect_hdr[FW_IMG_HDR_MAX_SIZE];

extern sec_boot_info_t sb_rom_info;
extern hal_timer_group_adapter_t _timer_group3;
extern hal_timer_adapter_t _fcs_system_timer;
extern sec_boot_keycerti_t export_sb_keycerti;
extern hal_sec_region_cfg_t sb_sec_cfg_pending;
extern sb_crypto_enc_info_t crypto_enc_info;
extern uint8_t aes_iv_info[2][4];
extern uint8_t aes_key_info[SB_SEC_KEY_SIZE];
extern uint8_t aes_gcm_tagfor_4k[512];

hal_status_t fw_spic_init(phal_spic_adaptor_t phal_spic_adaptor, u8 spic_bit_mode, u8 io_pin_sel);
hal_status_t fw_spic_deinit(phal_spic_adaptor_t phal_spic_adaptor);
void clear_export_partition_tbl_info(void);
void clear_export_sb_keycerti_info(void);
void clear_export_enc_sec_info(void);
int verify_manif_f(const uint8_t *img_offset, const uint8_t info_type, sec_boot_info_t *p_sb_info); //TOdo info_type may not use
void load_img_hdr_f(const uint8_t *img_offset, fw_img_hdr_t *img_hdr_buf, const uint8_t sel_img_load);
int load_img_sect_f(const uint8_t *img_offset, fw_img_hdr_t *pfw_hdr, sec_boot_info_t *p_sb_info);
uint8_t search_available_idx(uint8_t *p_sts, const uint8_t cfg_max_size);
int xip_pending_cfg_add_rmp(hal_sec_region_cfg_t *psb_sec_cfg_pending, img_region_lookup_t *pimg_rmp_lkp_tbl,
							uint32_t phy_addr, uint32_t remap_addr, uint32_t remap_sz);
int xip_pending_process_rmp(hal_sec_region_cfg_t *psb_sec_cfg_pending, sec_boot_info_t *p_sb_info);
int xip_disable_rmp_config(hal_sec_region_cfg_t *psb_sec_cfg_pending, uint8_t region_sel);
int xip_pending_cfg_add_dec_key(hal_sec_region_cfg_t *psb_sec_cfg_pending, const uint8_t key_id);
int xip_pending_cfg_add_dec(hal_sec_region_cfg_t *psb_sec_cfg_pending, img_region_lookup_t *pimg_dec_lkp_tbl,
							uint8_t cipher_sel, uint32_t dec_base, uint32_t dec_sz,
							uint32_t iv_ptn_low, uint32_t iv_ptn_high,
							uint32_t tag_base_addr, uint32_t tag_flh_addr, uint32_t total_hdr_size);
int xip_pending_process_dec(hal_sec_region_cfg_t *psb_sec_cfg_pending, sec_boot_info_t *p_sb_info);
int xip_disable_dec_config(hal_sec_region_cfg_t *psb_sec_cfg_pending, uint8_t region_sel);
int img_rmp_and_dec_lkp_tbl_insert(img_region_lookup_t *plkp_tbl, uint8_t tbl_size, uint8_t is_xip, uint8_t *tbl_cnt);
void img_rmp_and_dec_lkp_tbl_remove(img_region_lookup_t *plkp_tbl, uint8_t *tbl_cnt);
int img_get_ld_sel_info_from_ie(const uint8_t img_obj, const uint8_t *ptr, img_manifest_ld_sel_t *pld_sel_info);
uint32_t get_ld_version(const uint8_t img_obj, uint8_t *p_img_version);
uint32_t get_ld_timst(uint8_t *p_img_timest);
int img_get_ld_sel_idx(const uint8_t img_obj, img_manifest_ld_sel_t *pld_sel_info1, img_manifest_ld_sel_t *pld_sel_info2, uint8_t img1_idx, uint8_t img2_idx);
int img_get_update_sel_idx(const uint8_t img_obj, img_manifest_ld_sel_t *pld_sel_info1, img_manifest_ld_sel_t *pld_sel_info2, uint8_t img1_idx,
						   uint8_t img2_idx);
int img_sel_op_idx(void *p_tbl_info, const uint8_t img_obj, const uint8_t img_sel_op);

certi_tbl_t *fw_load_certi_tbl_direct_f(void);
partition_tbl_t *fw_load_partition_tbl_direct_f(void);
int verify_fcs_isp_iq_manif_f(const uint8_t *img_offset, const uint8_t info_type, uint8_t *pfcs_id);
int boot_rom_fcs_timer_init(hal_timer_group_adapter_t *ptimer_g_adptr, hal_timer_adapter_t *pfcs_system_timer);
int verify_certificateInfo_f(const uint8_t *ptr, sec_boot_info_t *p_sb_info, sec_boot_keycerti_t *psb_keycerti);
int sb_rom_img_vrf_op(const uint8_t sbl_cfg, sec_boot_info_t *p_sb_info, const uint8_t info_type);
hal_status_t sec_disable_dec_region(uint8_t dec_region_sel);
hal_status_t sec_default_calculate_tag_base(uint32_t flash_addr, uint32_t region_size, uint32_t tag_region_addr, uint32_t *tag_base, uint32_t *tag_region_size);
uint8_t sb_rom_sect_ld_hash_prechk_valid_f(sec_boot_info_t *p_sb_info);
int sb_rom_sect_ld_hash_chk_f(uint8_t *p_msg, uint32_t msglen, uint8_t *p_img_digest_chk, sec_boot_info_t *p_sb_info);
int boot_rom_set_sjtag_nonfixed_key(const uint8_t sjtag_obj, sec_boot_info_t *p_sb_info);
int sec_cfg_f_init(hal_sec_region_cfg_t *psb_sec_cfg_pending);
uint8_t chk_rom_img_type_id_invalid(const uint16_t type_id);
int sec_key_register_key_stg(sec_boot_info_t *p_sb_info);
void flash_sec_crypto_xor_data(unsigned char *buf1, unsigned char *buf2, uint32_t len, unsigned char *result);
void flash_sec_crypto_gen_iv(uint32_t flash_addr, uint32_t cahce_line_size, Flash_SEC_IV_Data_t *p_sec_iv, uint32_t iv_len);
int flash_sec_crypto_rom_init(uint8_t sec_mode, uint8_t *pIv_info, uint32_t iv_len, uint8_t key_idx, uint32_t key_len);
int flash_sec_crypto_rom_decrypt(uint32_t flash_addr, uint8_t *pMsg, uint32_t msglen, uint8_t *pResult, uint8_t *pflh_Tag);


#define __mem_dump(start,size,prefix) do{ \
    dbg_printf(prefix "\r\n"); \
    dump_for_one_bytes((uint8_t *)start,size); \
}while(0)

#endif  // end of "ROM_REGION"
#endif  // end of "#define _FW_FLASH_BOOT_H_"
