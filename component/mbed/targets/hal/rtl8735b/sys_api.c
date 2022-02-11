/**************************************************************************//**
 * @file     sys_api.c
 * @brief    This file implements system related API functions.
 *
 * @version  V1.00
 * @date     2021-11-02
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
#include "sys_api.h"
#include "hal_sys_ctrl.h"
#include "flash_api.h"
#include "snand_api.h"
#include "device_lock.h"

/**
  * @brief  system software reset.
  * @retval none
  */
void sys_reset(void)
{
	hal_sys_set_system_reset();
}

/**
  * @brief  Turn off the JTAG/SWD function.
  * @retval none
  */
void sys_jtag_off(void)
{
	hal_sys_dbg_port_cfg(DBG_PORT_OFF, TMS_IO_S0_CLK_S0);
	hal_sys_dbg_port_cfg(DBG_PORT_OFF, TMS_IO_S1_CLK_S1);
}

/**
  * @brief  Turn off the JTAG/SWD function.
  * @retval boot select device
  * @note
  *  BootFromNORFlash            = 0,
  *  BootFromNANDFlash           = 1,
  *  BootFromUART                = 2
  */
uint8_t sys_get_boot_sel(void)
{
	uint8_t boot_sel;
	boot_sel = hal_sys_get_boot_select();
	return boot_sel;
}

/**
  * @brief  Clear OTA signature so that boot code load default image.
  * @retval none
  */
void sys_clear_ota_signature(void)
{
	uint8_t cur_fw_idx = 0;
	uint8_t boot_sel = -1;

	cur_fw_idx = hal_sys_get_ld_fw_idx();
	if ((1 != cur_fw_idx) && (2 != cur_fw_idx)) {
		printf("\n\rcurrent fw index is wrong %d \n\r", cur_fw_idx);
		return;
	}

	boot_sel = sys_get_boot_sel();
	if (0 == boot_sel) {
		// boot from NOR flash

		flash_t flash;
		uint8_t label_init_value[8] = {0x52, 0x54, 0x4c, 0x38, 0x37, 0x33, 0x35, 0x42};
		uint8_t next_fw_label[8] = {0};
		uint32_t cur_fw_addr = 0, next_fw_addr = 0;
		uint8_t *pbuf = NULL;
		uint32_t buf_size = 4096;

		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (1 == cur_fw_idx) {
			// fw1 record in partition table
			flash_read_word(&flash, 0x2060, &cur_fw_addr);
			// fw2 record in partition table
			flash_read_word(&flash, 0x2080, &next_fw_addr);
		} else if (2 == cur_fw_idx) {
			// fw2 record in partition table
			flash_read_word(&flash, 0x2080, &cur_fw_addr);
			// fw1 record in partition table
			flash_read_word(&flash, 0x2060, &next_fw_addr);
		}
		flash_stream_read(&flash, next_fw_addr, 8, next_fw_label);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		if (0 != memcmp(next_fw_label, label_init_value, 8)) {
			printf("\n\rOnly one valid fw, no fw to clear");
			return;
		}

		//erase current FW signature to make it boot from another FW image
		printf("\n\rcurrent FW addr = 0x%08X", cur_fw_addr);

		pbuf = malloc(buf_size);
		if (!pbuf) {
			printf("\n\rAllocate buf fail");
			return;
		}

		// need to enter critical section to prevent executing the XIP code at first sector after we erase it.
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, cur_fw_addr, buf_size, pbuf);
		// NOT the first byte of ota signature to make it invalid
		pbuf[0] = ~(pbuf[0]);
		flash_erase_sector(&flash, cur_fw_addr);
		flash_burst_write(&flash, cur_fw_addr, buf_size, pbuf);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		free(pbuf);
	} else if (1 == boot_sel) {
		// boot from NAND flash

		//uint8_t partition_data[2112] __attribute__((aligned(32)));
		//uint8_t data_r[2112] __attribute__((aligned(32)));
		uint8_t *partition_data;
		uint8_t *data_r;
		partition_data = malloc(2112);
		data_r = malloc(2112);

		int update_partition_table = 0;

		snand_t flash;
		snand_init(&flash);

		device_mutex_lock(RT_DEV_LOCK_FLASH);

		//read partition_table block16-23
		for (int i = 16; i < 24; i++) {
			snand_page_read(&flash, i * 64, 2048 + 8, &partition_data[0]);
			if (partition_data[2048] == 0xff) {
				break;
			}
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		if (1 == cur_fw_idx) {
			for (int i = 0; i < 16; i++) {
				if ((partition_data[i * 128] == 0x87) && (partition_data[i * 128 + 1] == 0xff) && (partition_data[i * 128 + 2] == 0x35) &&
					(partition_data[i * 128 + 3] == 0xff) && (partition_data[i * 128 + 4] == 0xc8) && (partition_data[i * 128 + 5] == 0xb9)) {
					printf("partition_table FW2 type_id is valid \n\r");
					update_partition_table = 1;
				}
			}
			if (update_partition_table == 1) {
				for (int i = 0; i < 16; i++) {
					if ((partition_data[i * 128] == 0x87) && (partition_data[i * 128 + 1] == 0xff) && (partition_data[i * 128 + 2] == 0x35) &&
						(partition_data[i * 128 + 3] == 0xff) && (partition_data[i * 128 + 4] == 0xc7) && (partition_data[i * 128 + 5] == 0xc1)) {
						printf("clear partition_table FW1 magic_num \n\r");
						partition_data[i * 128] = 0x0; //0x87 to 0x0
						partition_data[i * 128 + 2] = 0x0; //0x35 to 0x0
					}
				}
			}
		} else if (2 == cur_fw_idx) {
			for (int i = 0; i < 16; i++) {
				if ((partition_data[i * 128] == 0x87) && (partition_data[i * 128 + 1] == 0xff) && (partition_data[i * 128 + 2] == 0x35) &&
					(partition_data[i * 128 + 3] == 0xff) && (partition_data[i * 128 + 4] == 0xc7) && (partition_data[i * 128 + 5] == 0xc1)) {
					printf("partition_table FW1 type_id is valid \n\r");
					update_partition_table = 1;
				}
			}
			if (update_partition_table == 1) {
				for (int i = 0; i < 16; i++) {
					if ((partition_data[i * 128] == 0x87) && (partition_data[i * 128 + 1] == 0xff) && (partition_data[i * 128 + 2] == 0x35) &&
						(partition_data[i * 128 + 3] == 0xff) && (partition_data[i * 128 + 4] == 0xc8) && (partition_data[i * 128 + 5] == 0xb9)) {
						printf("clear partition_table FW2 magic_num \n\r");
						partition_data[i * 128] = 0x0; //0x87 to 0x0
						partition_data[i * 128 + 2] = 0x0; //0x35 to 0x0
					}
				}
			}
		}

		//update partition table block16-23
		if (update_partition_table == 1) {
			int success = 0;
			int fail = 0;
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			for (int i = 16; i < 24; i++) {
				fail = 0;
				snand_erase_block(&flash, i * 64);
				snand_page_write(&flash, i * 64, 2048 + 8, &partition_data[0]);
				snand_page_read(&flash, i * 64, 2048 + 8, &data_r[0]);
				if (memcmp(partition_data, data_r, (2048 + 8)) != 0) {
					printf("bolck %d write fail! \n\r", i);
					fail = 1;
					snand_erase_block(&flash, i * 64);
					data_r[2048] = 0;
					snand_page_write(&flash, i * 64, 2048 + 8, &data_r[0]);
				}
				if (fail == 0) {
					success = success + 1;
				}
				if (success == 2) {
					break;
				}
			}
			device_mutex_unlock(RT_DEV_LOCK_FLASH);

		}
		free(partition_data);
		free(data_r);

	}

	printf("\n\rClear OTA signature success.");
}

/**
  * @brief  Recover OTA signature so that boot code load upgraded image(ota image).
  * @retval none
  */
void sys_recover_ota_signature(void)
{
	uint8_t cur_fw_idx = 0;
	uint8_t boot_sel = -1;

	cur_fw_idx = hal_sys_get_ld_fw_idx();
	if ((1 != cur_fw_idx) && (2 != cur_fw_idx)) {
		printf("\n\rcurrent fw index is wrong %d \n\r", cur_fw_idx);
		return;
	}

	boot_sel = sys_get_boot_sel();
	if (0 == boot_sel) {
		// boot from NOR flash

		flash_t flash;
		uint8_t label_init_value[8] = {0x52, 0x54, 0x4c, 0x38, 0x37, 0x33, 0x35, 0x42};
		uint8_t target_fw_label[8] = {0};
		uint32_t target_fw_addr = 0;
		uint8_t *pbuf = NULL;
		uint32_t buf_size = 4096;

		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (1 == cur_fw_idx) {
			// fw2 record in partition table
			flash_read_word(&flash, 0x2080, &target_fw_addr);
		} else if (2 == cur_fw_idx) {
			// fw1 record in partition table
			flash_read_word(&flash, 0x2060, &target_fw_addr);
		}
		flash_stream_read(&flash, target_fw_addr, 8, target_fw_label);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		// check next fw label
		if (0 == memcmp(target_fw_label, label_init_value, 8)) {
			printf("\n\rBoth fw valid, no fw to recover");
			return;
		}

		printf("\n\rtarget  FW addr = 0x%08X", target_fw_addr);

		pbuf = malloc(buf_size);
		if (!pbuf) {
			printf("\n\rAllocate buf fail");
			return;
		}

		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, target_fw_addr, buf_size, pbuf);
		// NOT the first byte of ota signature to make it valid
		pbuf[0] = ~(pbuf[0]);
		flash_erase_sector(&flash, target_fw_addr);
		flash_burst_write(&flash, target_fw_addr, buf_size, pbuf);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		free(pbuf);
	} else if (1 == boot_sel) {
		// boot from NAND flash
		//uint8_t partition_data[2112] __attribute__((aligned(32)));
		//uint8_t data_r[2112] __attribute__((aligned(32)));
		uint8_t *partition_data;
		uint8_t *data_r;
		partition_data = malloc(2112);
		data_r = malloc(2112);

		int update_partition_table = 0;

		snand_t flash;
		snand_init(&flash);

		device_mutex_lock(RT_DEV_LOCK_FLASH);

		//read partition_table block16-23
		for (int i = 16; i < 24; i++) {
			snand_page_read(&flash, i * 64, 2048 + 8, &partition_data[0]);
			if (partition_data[2048] == 0xff) {
				break;
			}
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		if (1 == cur_fw_idx) {
			for (int i = 0; i < 16; i++) {
				if ((partition_data[i * 128 + 4] == 0xc8) && (partition_data[i * 128 + 5] == 0xb9)) {
					printf("partition_table FW2 type_id is valid \n\r");
					partition_data[i * 128] = 0x87; //0x0 to 0x87
					partition_data[i * 128 + 2] = 0x35; //0x0 to 0x35
					printf("recover partition_table FW2 magic_num \n\r");
					update_partition_table = 1;
				}
			}
		} else if (2 == cur_fw_idx) {
			for (int i = 0; i < 16; i++) {
				if ((partition_data[i * 128 + 4] == 0xc7) && (partition_data[i * 128 + 5] == 0xc1)) {
					printf("partition_table FW1 type_id is valid \n\r");
					partition_data[i * 128] = 0x87; //0x0 to 0x87
					partition_data[i * 128 + 2] = 0x35; //0x0 to 0x35
					printf("recover partition_table FW1 magic_num \n\r");
					update_partition_table = 1;
				}
			}
		}

		//update partition table block16-23
		if (update_partition_table == 1) {
			int success = 0;
			int fail = 0;
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			for (int i = 16; i < 24; i++) {
				fail = 0;
				snand_erase_block(&flash, i * 64);
				snand_page_write(&flash, i * 64, 2048 + 8, &partition_data[0]);
				snand_page_read(&flash, i * 64, 2048 + 8, &data_r[0]);
				if (memcmp(partition_data, data_r, (2048 + 8)) != 0) {
					printf("bolck %d write fail! \n\r", i);
					fail = 1;
					snand_erase_block(&flash, i * 64);
					data_r[2048] = 0;
					snand_page_write(&flash, i * 64, 2048 + 8, &data_r[0]);
				}
				if (fail == 0) {
					success = success + 1;
				}
				if (success == 2) {
					break;
				}
			}
			device_mutex_unlock(RT_DEV_LOCK_FLASH);

		}
		free(partition_data);
		free(data_r);
	}

	printf("\n\rRecover OTA signature success.");
}

