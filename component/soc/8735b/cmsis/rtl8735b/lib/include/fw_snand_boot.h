/**************************************************************************//**
 * @file     fw_snand_boot.h
 * @brief    Declare the booting from nand flash.
 *
 * @version  V1.00
 * @date     2021-07-26
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

#ifndef _FW_SNAND_BOOT_H_
#define _FW_SNAND_BOOT_H_

#include "fw_img_tlv.h"
#include "rtl8735b_ramstart.h"
#include "cmsis.h"
#include "static_assert.h"

/* Util macro */
#define SIZE_ALIGN_TO(size, align)  (size % align > 0? (((size / align) + 1) * align): size)

/* End of util */

#define NAND_PAGE_MAX_LEN           (2 * 0x840)

#define NAND_TMP_DATA_MAX_LEN       0x1000

#define IMG_HDR_MAX                 256
#define SECT_HDR_MAX                256

/// Fixed ctrl info start blk
#define NAND_CTRL_INFO_START_BLK    0
/// Fixed ctrl info backup count
#define NAND_CTRL_INFO_DUP_CNT      8
/// Max count of virtual block map
#define SNAND_VMAP_MAX              48
/// SNAND start block for debug
#define DBG_SNAND_START_BLK         0

#define SNAND_PAR_RECORD_MAG_NUM    0xFF35FF87

#define NAND_SPARE_TYPE_IDX         4

#define SNAND_INVAL_PAR_TBL_IDX     ((u8)0xFF)

#define SNAND_ADDR_INIT(PAGE, COL)  { .page = PAGE, .col = COL}
#define SNAND_ADDR_BLK_INIT(BLK, COL)  { .page = (BLK) * NAND_PAGE_PER_BLK, .col = COL}

#define SNAND_ADDR_SET(addr_ptr, PAGE, COL) {(addr_ptr)->page = PAGE; (addr_ptr)->col = COL; }
#define SNAND_ADDR_BLK_SET(addr_ptr, BLK, COL) \
    {(addr_ptr)->page = (BLK) * NAND_PAGE_PER_BLK; \
     (addr_ptr)->col = COL; }

// Please use with
#define SNAND_VADDR_SET(addr_ptr, PAGE, COL, SIZE) \
    {(addr_ptr)->page = PAGE; \
    (addr_ptr)->col = COL; \
    (addr_ptr)->map_size = SIZE;}

/// NAND flash page data size in byte
#define NAND_PAGE_LEN               ((const u32)NAND_CTRL_INFO.page_size)
/// NAND flash page spare size in byte
#define NAND_SPARE_LEN              ((const u32)NAND_CTRL_INFO.spare_size)
/// NAND flash page count per block
#define NAND_PAGE_PER_BLK           ((const u32)NAND_CTRL_INFO.page_per_blk)
/// Total block count in flash
#define NAND_BLK_CNT                ((const u32)NAND_CTRL_INFO.blk_cnt)
/// NAND flash block size in byte
#define NAND_BLK_LEN                (NAND_PAGE2ADDR(NAND_PAGE_PER_BLK))
/// Get offset of address inside page
#define NAND_PAGE_OFST(addr)        ((addr) & NAND_CTRL_INFO.page_mask)
/// Get offset of page inside block
#define NAND_BLK_OFST(page_addr)    ((page_addr) & NAND_CTRL_INFO.blk_mask)
/// Convert page to block
#define NAND_PAGE2BLK(page_addr)    ((page_addr) >> NAND_CTRL_INFO.page2blk_shift)
/// Convert address to page
#define NAND_ADDR2PAGE(addr)        ((addr) >> NAND_CTRL_INFO.addr2page_shift)
/// Convert page to address
#define NAND_PAGE2ADDR(page_addr)   ((page_addr) << NAND_CTRL_INFO.addr2page_shift)
/// Convert block to page
#define NAND_BLK2PAGE(blk_idx)      ((blk_idx) << NAND_CTRL_INFO.page2blk_shift)
/// Base address of simulated snand address
#define NAND_SIM_ADDR_BASE          SPI_FLASH_BASE
/// Convert page & col to simulated address, do not directly access the address
#define NAND_SIM_ADDR(addr_ptr)    ((void *)(NAND_PAGE2ADDR((addr_ptr)->page) + (addr_ptr)->col + NAND_SIM_ADDR_BASE))

#define CEILING_NBYTE(SIZE, N)      (SIZE + ((SIZE % N)  ? (N - (SIZE % N)) : 0))

typedef u16 snand_vblk_idx_t;

typedef struct snand_raw_ctrl_info {
	u32 blk_cnt;
	u32 page_per_blk;
	u32 page_size;
	u32 spare_size;
	u32 vendor_info[4];
	u32 par_tbl_start;
	u32 par_tbl_dup_cnt;
	u16 vrf_alg;            // verify image algorithm selection
	u16 rsvd1;
	u32 rsvd2[13];
} snand_raw_ctrl_info_t;

STATIC_ASSERT(sizeof(snand_raw_ctrl_info_t) % 32 == 0, struct_not_align);

typedef struct snand_ctrl_info {
	u32 blk_cnt;
	u32 page_per_blk;
	u32 page_size;
	u32 spare_size;
	u32 page_mask;
	u32 blk_mask;
	u8 page2blk_shift;
	u8 addr2page_shift;
	u8 cache_rsvd[6];
	// cache line size
#if IS_AFTER_CUT_B(CONFIG_CHIP_VER)
	snand_raw_ctrl_info_t raw;
	u32 rsvd2[16];
#else
	u32 rsvd2[16 + (sizeof(snand_raw_ctrl_info_t) / sizeof(u32))];
#endif
} snand_ctrl_info_t;

STATIC_ASSERT((offsetof(snand_ctrl_info_t, cache_rsvd) + sizeof(((snand_ctrl_info_t *)NULL)->cache_rsvd)) == 32, struct_not_align);
STATIC_ASSERT(sizeof(snand_ctrl_info_t) % 32 == 0, struct_not_align);

typedef struct snand_addr {
	u32 page;
	u16 col;
} snand_addr_t;

typedef struct snand_vaddr {
	u32 page;
	u16 col;
	u8 map_size;
	u16 vmap[SNAND_VMAP_MAX];          // virtual block to physical block mapping
} snand_vaddr_t;

// Partition record in NAND flash layout
typedef struct snand_part_record {
	u32 magic_num;
	u16 type_id;
	u16 blk_cnt;
	u32 rsvd[6];
	snand_vblk_idx_t vblk[SNAND_VMAP_MAX];
} snand_part_record_t;

// partition table info
typedef struct snand_part_entry {
	u16 type_id;
	u16 blk_cnt;
	snand_addr_t vmap_addr;
	u32 rsvd[6];
} snand_part_entry_t;

typedef struct snand_partition_tbl {
	part_fst_info_t mfst;
	snand_part_entry_t entrys[PARTITION_RECORD_MAX];
} snand_partition_tbl_t;

typedef struct hal_snand_boot_stubs {
	u8 *rom_snand_boot;
	snand_ctrl_info_t *ctrl_info;
	snand_partition_tbl_t *part_tbl;
	hal_status_t (*snand_memcpy_update)(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_addr_t *snand_addr, u32 size);
	hal_status_t (*snand_memcpy)(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_addr_t *snand_addr, u32 size);
	hal_status_t (*snand_offset)(snand_addr_t *snand_addr, u32 offset);
	hal_status_t (*snand_vmemcpy_update)(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_vaddr_t *snand_addr, u32 size);
	hal_status_t (*snand_vmemcpy)(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_vaddr_t *snand_addr, u32 size);
	hal_status_t (*snand_voffset)(snand_vaddr_t *snand_addr, u32 offset);
#if !IS_AFTER_CUT_B(CONFIG_CHIP_VER)
	u32 rsvd[39];
#else
	u8(*snand_img_sel)(snand_partition_tbl_t *part_tbl, const u8 img_obj, const u8 img_sel_op);
	u32 rsvd[38];
#endif
} hal_snand_boot_stubs_t;

STATIC_ASSERT(sizeof(hal_snand_boot_stubs_t) == (48 * 4), stub_sz_changed);

extern const hal_snand_boot_stubs_t hal_snand_boot_stubs;

#if defined(ROM_REGION)

#define NAND_CTRL_INFO              rom_snand_ctrl_info
extern snand_ctrl_info_t rom_snand_ctrl_info;
extern u8 rom_snand_boot;
extern snand_partition_tbl_t snand_part_tbl;
extern u8 snand_page_buf[NAND_PAGE_MAX_LEN]; // must > 4K to store manifest
extern hal_snafc_adaptor_t boot_snafc_adpt;

hal_status_t fw_spic_pinmux_ctl(phal_spic_adaptor_t, flash_pin_sel_t *, u8);
hal_status_t fw_snafc_deinit(hal_snafc_adaptor_t *pSnafcAdaptor);
s32 snand_flash_boot(PRAM_FUNCTION_START_TABLE *pram_start_func);
void init_ctrl_info(snand_ctrl_info_t *info);

hal_status_t snand_memcpy(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_addr_t *snand_addr, u32 size);
hal_status_t snand_memcpy_update(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_addr_t *snand_addr, u32 size);
hal_status_t snand_offset(snand_addr_t *snand_addr, u32 offset);
hal_status_t snand_vmemcpy_update(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_vaddr_t *snand_addr, u32 size);
hal_status_t snand_vmemcpy(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_vaddr_t *snand_addr, u32 size);
hal_status_t snand_voffset(snand_vaddr_t *snand_addr, u32 offset);
hal_status_t snand_sim_memcpy(void *dst, const void *src, u32 size);
s32 sb_snand_hash_update(hal_crypto_adapter_t *crypto_adtr, uint32_t msglen, uint32_t auth_type, uint8_t *p_msg, uint8_t is_flash_dat);
u8 snand_img_sel(snand_partition_tbl_t *part_tbl, const u8 img_obj, const u8 img_sel_op);
hal_status_t fw_snafc_init_core(hal_snafc_adaptor_t *snafc_adpt, u8 spic_bit_mode, u8 io_pin_sel, u8 do_protect);

__STATIC_FORCEINLINE
hal_status_t fw_snafc_init_wr(hal_snafc_adaptor_t *snafc_adpt, u8 spic_bit_mode, u8 io_pin_sel)
{
	// Write Proect disable
	return fw_snafc_init_core(snafc_adpt, spic_bit_mode, io_pin_sel, DISABLE);
}

__STATIC_FORCEINLINE
hal_status_t fw_snafc_init(hal_snafc_adaptor_t *snafc_adpt, u8 spic_bit_mode, u8 io_pin_sel)
{
	return fw_snafc_init_core(snafc_adpt, spic_bit_mode, io_pin_sel, ENABLE);
}

#elif defined(CONFIG_BUILD_BOOT) && (CONFIG_BUILD_BOOT == 1) // Boor laoder only

//#define NAND_CTRL_INFO              snand_ctrl_info
//extern snand_ctrl_info_t snand_ctrl_info;
#define NAND_CTRL_INFO              (*hal_snand_boot_stubs.ctrl_info)

extern hal_snafc_adaptor_t bl_snafc_adpt;

s32 snand_boot_loader(PRAM_FUNCTION_START_TABLE *ram_start_func);
s32 snand_load_single_img(const fw_img_hdr_t *img_hdr, snand_vaddr_t *cur_addr,
						  const snand_vaddr_t *img_base_addr, sec_boot_info_t *sb_info,
						  PRAM_FUNCTION_START_TABLE *ram_start_func, u32 dec_base);

#if 0
__STATIC_FORCEINLINE hal_status_t snand_memcpy(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_addr_t *snand_addr, u32 size)
{
	return hal_snand_boot_stubs.snand_memcpy(snafc_adpt, dest, snand_addr, size);
}

__STATIC_FORCEINLINE hal_status_t snand_memcpy_update(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_addr_t *snand_addr, u32 size)
{
	return hal_snand_boot_stubs.snand_memcpy_update(snafc_adpt, dest, snand_addr, size);
}
__STATIC_FORCEINLINE hal_status_t snand_vmemcpy_update(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_vaddr_t *snand_addr, u32 size)
{
	return hal_snand_boot_stubs.snand_vmemcpy_update(snafc_adpt, dest, snand_addr, size);
}

__STATIC_FORCEINLINE hal_status_t snand_vmemcpy(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_vaddr_t *snand_addr, u32 size)
{
	return hal_snand_boot_stubs.snand_vmemcpy(snafc_adpt, dest, snand_addr, size);
}

__STATIC_FORCEINLINE hal_status_t snand_offset(snand_addr_t *snand_addr, u32 offset)
{
	return hal_snand_boot_stubs.snand_offset(snand_addr, offset);
}

__STATIC_FORCEINLINE hal_status_t snand_voffset(snand_vaddr_t *snand_addr, u32 offset)
{
	return hal_snand_boot_stubs.snand_voffset(snand_addr, offset);
}
#else
hal_status_t snand_memcpy(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_addr_t *snand_addr, u32 size);
hal_status_t snand_memcpy_update(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_addr_t *snand_addr, u32 size);
hal_status_t snand_vmemcpy_update(hal_snafc_adaptor_t *snafc_adpt, void *dest, snand_vaddr_t *snand_addr, u32 size);
hal_status_t snand_vmemcpy(hal_snafc_adaptor_t *snafc_adpt, void *dest, const snand_vaddr_t *snand_addr, u32 size);
hal_status_t snand_offset(snand_addr_t *snand_addr, u32 offset);
hal_status_t snand_voffset(snand_vaddr_t *snand_addr, u32 offset);
#endif

#if IS_AFTER_CUT_C(CONFIG_CHIP_VER)
// FIXME Update rom function
__STATIC_FORCEINLINE u8 snand_img_sel(snand_partition_tbl_t *part_tbl, const u8 img_obj, const u8 img_sel_op)
{
	return hal_snand_boot_stubs.snand_img_sel(part_tbl, img_obj, img_sel_op);
}
#else
u8 snand_img_sel(snand_partition_tbl_t *part_tbl, const u8 img_obj, const u8 img_sel_op);
#endif

#endif

#endif  // end of "#define _FW_SNAND_BOOT_H_"
