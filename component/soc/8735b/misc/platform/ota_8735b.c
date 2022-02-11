#include <sys.h>
#include <device_lock.h>
#include "ota_8735b.h"
#include "lwipconf.h"
#include "sys_api.h"
#include "osdep_service.h"
#include "flash_api.h"
#include "snand_api.h"

sys_thread_t TaskOTA = NULL;
#define STACK_SIZE		1024
#define TASK_PRIORITY	tskIDLE_PRIORITY + 1

// Checksum check before make fw valid
// Please make sure target OTA firmware did contains 4 bytes checksum value, or the checksum check would always fail
// User can check target firmware postbuild routine.
#define USE_CHECKSUM 1

int ota_flash_NOR(uint32_t target_fw_idx, uint32_t total_blocks, uint32_t cur_block, uint8_t *buf, uint32_t data_len, _file_checksum file_checksum)
{
	static uint32_t target_fw_addr = 0;
	static uint32_t target_fw_len = 0;
#if USE_CHECKSUM
	static uint32_t flash_checksum = 0;
#endif
	static uint8_t label_backup[8];
	uint8_t label_readback[8];
	flash_t flash;
	int ret = 0;

	// for first block
	if (0 == cur_block) {
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (1 == target_fw_idx) {
			// fw1 record in partition table
			flash_read_word(&flash, 0x2060, &target_fw_addr);
			flash_read_word(&flash, 0x2064, &target_fw_len);
		} else if (2 == target_fw_idx) {
			// fw2 record in partition table
			flash_read_word(&flash, 0x2080, &target_fw_addr);
			flash_read_word(&flash, 0x2084, &target_fw_len);
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		printf("\n\r[%s] target_fw_addr=0x%x, target_fw_len=0x%x\n\r", __FUNCTION__, target_fw_addr, target_fw_len);
		if ((total_blocks * NOR_BLOCK_SIZE) > target_fw_len) {
			ret = -1;
			goto exit;
		}

		memcpy(label_backup, buf, 8); // save 8-bytes fw label
		memset(buf, 0xFF, 8); // not flash write 8-bytes fw label
#if USE_CHECKSUM
		flash_checksum = 0;
#endif
	}

	// flash write
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash, target_fw_addr + cur_block * NOR_BLOCK_SIZE);
	if (flash_burst_write(&flash, target_fw_addr + cur_block * NOR_BLOCK_SIZE, data_len, buf) < 0) {
		printf("\n\r[%s] flash write failed", __FUNCTION__);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		ret = -1;
		goto exit;
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

#if USE_CHECKSUM
	// read flash data back and calculate checksum
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, target_fw_addr + cur_block * NOR_BLOCK_SIZE, data_len, buf);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	for (int i = 0; i < data_len; i ++) {
		if ((0 == cur_block) && (i < 8)) {
			flash_checksum += label_backup[i];
		} else {
			flash_checksum += buf[i];
		}
	}
#endif

	// for final block
	if (cur_block == (total_blocks - 1)) {
#if USE_CHECKSUM
		printf("\n\rflash checksum 0x%8x attached checksum 0x%8x", flash_checksum, file_checksum.u);

		if (file_checksum.u != flash_checksum) {
			printf("\n\r[%s] The checksume is wrong!\n\r", __FUNCTION__);
			ret = -1;
			goto exit;
		}
#endif
		// write back 8-bytes fw label
		printf("\n\r[%s] Append FW label", __FUNCTION__);
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (flash_burst_write(&flash, target_fw_addr, 8, label_backup) < 0) {
			printf("\n\r[%s] flash write failed", __FUNCTION__);
			ret = -1;
		}
		flash_stream_read(&flash, target_fw_addr, 8, label_readback);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		printf("\n\r[%s] FW label:\n\r", __FUNCTION__);
		for (int i = 0; i < 8; i ++) {
			printf(" %02X", label_readback[i]);
		}
		printf("\n\r");
	}

exit:

	return ret;
}

int ota_flash_NAND(uint32_t target_fw_idx, uint32_t total_blocks, uint32_t cur_block, uint8_t *buf, uint32_t data_len, _file_checksum file_checksum)
{
#if USE_CHECKSUM
	static uint32_t flash_checksum = 0;
#endif
	static uint8_t partition_data[2112] __attribute__((aligned(32)));
//	uint8_t data[2112] __attribute__((aligned(32)));
//	uint8_t data_r[2112] __attribute__((aligned(32)));
//	uint8_t *partition_data;
	uint8_t *data;
	uint8_t *data_r;
//	partition_data = update_malloc(2112);
	data = update_malloc(2112);
	data_r = update_malloc(2112);

	int block_w_nub = 24;//start  form 24
	int block_used = 0 ;
	int success = 0;
	int fail = 0;
	int ret = 0;
	int partition_valid_block = 0 ;
	snand_t flash;
	snand_init(&flash);



	// for first block
	if (0 == cur_block) {
		//read partition_table block16-23
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		for (int i = 16; i < 24; i++) {
			snand_page_read(&flash, i * 64, 2048 + 32, &data_r[0]);
			if ((data_r[2048] == 0xff) && (data_r[2053] == 0xc4) && (data_r[2054] == 0xd9)) {
				memcpy(partition_data, data_r, 2112);
				partition_valid_block = i;
				success = success + 1;
			}
			if (success == 2) {
				break;
			}
			if (i == 23) {
				printf("\n\r[%s] Read partition_table is wrong!\n\r", __FUNCTION__);
				ret = -1;
				goto exit;

			}
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		//check have 2 partition_table
		if (success == 1) {
			printf("Just 1 partition_table! \n\r");
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			for (int i = 16; i < 24; i++) {
				if (i != partition_valid_block) {
					snand_erase_block(&flash, i * 64);
					snand_page_write(&flash, i * 64, 2048 + 32, &partition_data[0]);
					snand_page_read(&flash, i * 64, 2048 + 32, &data_r[0]);
					if (memcmp(partition_data, data_r, (2048 + 8)) != 0) {
						printf("bolck %d write fail! \n\r", i);
						snand_erase_block(&flash, i * 64);
						data_r[2048] = 0;
						snand_page_write(&flash, i * 64, 2048 + 32, &data_r[0]);
					} else { //success
						break;
					}
				}
			}
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
		}

#if USE_CHECKSUM
		flash_checksum = 0;
#endif
		memset(data, 0xff, 2112);
		int record_nub = 0;
		if (1 == target_fw_idx) {
			// clear fw1 record in partition table
			for (int i = 0; i < 16; i++) {
				if ((partition_data[i * 128 + 4] != 0xc7) && (partition_data[i * 128 + 5] != 0xc1)) {
					memcpy(&data[record_nub * 128], &partition_data[i * 128], 128);
					record_nub = record_nub + 1;
				}
			}
		} else if (2 == target_fw_idx) {
			// clear fw2 record in partition table
			for (int i = 0; i < 16; i++) {
				if ((partition_data[i * 128 + 4] != 0xc8) && (partition_data[i * 128 + 5] != 0xb9)) {
					memcpy(&data[record_nub * 128], &partition_data[i * 128], 128);
					record_nub = record_nub + 1;
				}
			}
		} else {
			printf("\n\r[%s] The target_fw_idx is wrong!\n\r", __FUNCTION__);
			ret = -1;
			goto exit;
		}
		//update partition_table
		memcpy(partition_data, data, 2048);
		//update partition table block16-23
		success = 0;
		fail = 0;
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		for (int i = 16; i < 24; i++) {
			fail = 0;
			snand_erase_block(&flash, i * 64);
			snand_page_write(&flash, i * 64, 2048 + 32, &partition_data[0]);
			snand_page_read(&flash, i * 64, 2048 + 32, &data_r[0]);
			if (memcmp(partition_data, data_r, (2048 + 8)) != 0) {
				printf("bolck %d write fail! \n\r", i);
				fail = 1;
				snand_erase_block(&flash, i * 64);
				data_r[2048] = 0;
				snand_page_write(&flash, i * 64, 2048 + 32, &data_r[0]);
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
//	for (int i = 0; i < 2048; i++) {
//		printf("[%d]= 0x%x\n\r",i,partition_data[i]);
//	}
	// find a empty block
	for (int i = block_w_nub; i < 2048; i++) {
		for (int j = 0; j < 16; j++) {
			for (int k = 0; k < 48; k++) {
				if (block_w_nub == ((partition_data[j * 128 + 32 + k * 2 + 1] << 8) + partition_data[j * 128 + 32 + k * 2])) {
					//printf("block %d is used\n\r", block_w_nub);
					block_w_nub = block_w_nub + 1;
					block_used = 1 ;
					break;
				}
			}
			if (block_used == 1) {
				break;
			}
		}
		if (block_used == 0) {
			printf("block %d is empty\n\r", block_w_nub);
			// flash write
			memset(data, 0xff, 2112);
			data[2048 + 1] = 0x87;
			data[2048 + 3] = 0x35;
			if (1 == target_fw_idx) {
				data[2048 + 5] = 0xc7;
				data[2048 + 6] = 0xc1;
			} else if (2 == target_fw_idx) {
				data[2048 + 5] = 0xc8;
				data[2048 + 6] = 0xb9;
			}

			device_mutex_lock(RT_DEV_LOCK_FLASH);
			fail = 0;
			snand_erase_block(&flash, i * 64);
			for (int j = 0; j < 64; j++) {
				memcpy(data, &buf[2048 * j], 2048);
				snand_page_write(&flash, block_w_nub * 64 + j, 2048 + 32, &data[0]);
				snand_page_read(&flash, block_w_nub * 64 + j, 2048 + 32, &data_r[0]);
				if (memcmp(data, data_r, (2048 + 8)) != 0) {
					printf("bolck %d write fail! \n\r", i);
					fail = 1;
					snand_erase_block(&flash, block_w_nub * 64);
					data_r[2048] = 0;
					snand_page_write(&flash, block_w_nub * 64, 2048 + 32, &data_r[0]);
					block_w_nub = block_w_nub + 1;
					break;
				}
			}
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			if (fail == 0) {
				break;    //success
			}
		}
		block_used = 0 ;
		if (i == 2047) {
			printf("all block is not empty , OTA fail!\n\r");
			ret = -1;
			goto exit;
		}
	}

#if USE_CHECKSUM
	for (int i = 0; i < data_len; i ++) {
		flash_checksum += buf[i];
	}
	// for final block
	if (cur_block == (total_blocks - 1)) {
		printf("\n\rflash checksum 0x%8x attached checksum 0x%8x", flash_checksum, file_checksum.u);
		if (file_checksum.u != flash_checksum) {
			printf("\n\r[%s] The checksume is wrong!\n\r", __FUNCTION__);
			ret = -1;
			goto exit;
		}
	}
#endif

	printf("block %d write success\n\r", block_w_nub);

	//update partition_table
	if ((cur_block % 48) == 0) {
		for (int i = 0; i < 16; i++) {
			if ((partition_data[i * 128 + 4] == 0xff) && (partition_data[i * 128 + 5] == 0xff)) {
				printf("partition_table record %d is empty \n\r", i);
				if (1 == target_fw_idx) {
					partition_data[i * 128 + 4] = 0xc7;
					partition_data[i * 128 + 5] = 0xc1;
				} else if (2 == target_fw_idx) {
					partition_data[i * 128 + 4] = 0xc8;
					partition_data[i * 128 + 5] = 0xb9;
				} else {
					printf("\n\r[%s] The target_fw_idx is wrong!\n\r", __FUNCTION__);
					ret = -1;
					goto exit;
				}

				partition_data[i * 128 + 6] = total_blocks & 0xff;
				partition_data[i * 128 + 7] = (total_blocks >> 8) & 0xff;
				partition_data[i * 128 + 0x20] = block_w_nub & 0xff;
				partition_data[i * 128 + 0x20 + 1] = (block_w_nub >> 8) & 0xff;
				break;
			}
		}
	} else {
		for (int i = 0; i < 16; i++) {
			if (1 == target_fw_idx) {
				if ((partition_data[i * 128 + 4] == 0xc7) && (partition_data[i * 128 + 5] == 0xc1)) {
					for (int j = 0; j < 48; j++) {
						if ((partition_data[(i * 128) + 0x20 + (2 * j)] == 0xff) && (partition_data[(i * 128) + 0x20 + (2 * j) + 1] == 0xff)) {
							partition_data[(i * 128) + 0x20 + (2 * j)] = block_w_nub & 0xff;
							partition_data[(i * 128) + 0x20 + (2 * j) + 1] = (block_w_nub >> 8) & 0xff;
							break;
						}
					}
				}
			} else if (2 == target_fw_idx) {
				if ((partition_data[i * 128 + 4] == 0xc8) && (partition_data[i * 128 + 5] == 0xb9)) {
					for (int j = 0; j < 48; j++) {
						if ((partition_data[(i * 128) + 0x20 + (2 * j)] == 0xff) && (partition_data[(i * 128) + 0x20 + (2 * j) + 1] == 0xff)) {
							partition_data[(i * 128) + 0x20 + (2 * j)] = block_w_nub & 0xff;
							partition_data[(i * 128) + 0x20 + (2 * j) + 1] = (block_w_nub >> 8) & 0xff;
							break;
						}
					}
				}
			}
		}
	}

	// for final block update magic number
	if (cur_block == (total_blocks - 1)) {
		for (int i = 0; i < 16; i++) {
			if (1 == target_fw_idx) {
				if ((partition_data[i * 128 + 4] == 0xc7) && (partition_data[i * 128 + 5] == 0xc1)) {
					partition_data[i * 128] = 0x87;
					partition_data[i * 128 + 2] = 0x35;
					printf("partition_table update magic number OK\n\r");
				}
			} else if (2 == target_fw_idx) {
				if ((partition_data[i * 128 + 4] == 0xc8) && (partition_data[i * 128 + 5] == 0xb9)) {
					partition_data[i * 128] = 0x87;
					partition_data[i * 128 + 2] = 0x35;
					printf("partition_table update magic number OK\n\r");
				}
			}
		}

		//update partition table block16-23
		success = 0;
		fail = 0;
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		for (int i = 16; i < 24; i++) {
			fail = 0;
			snand_erase_block(&flash, i * 64);
			snand_page_write(&flash, i * 64, 2048 + 32, &partition_data[0]);
			snand_page_read(&flash, i * 64, 2048 + 32, &data_r[0]);
			if (memcmp(partition_data, data_r, (2048 + 8)) != 0) {
				printf("bolck %d write fail! \n\r", i);
				fail = 1;
				snand_erase_block(&flash, i * 64);
				data_r[2048] = 0;
				snand_page_write(&flash, i * 64, 2048 + 32, &data_r[0]);
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


exit:
//	update_free(partition_data);
	update_free(data);
	update_free(data_r);
	return ret;
}

void *update_malloc(unsigned int size)
{
	return pvPortMalloc(size);
}

void update_free(void *buf)
{
	vPortFree(buf);
}

void ota_platform_reset(void)
{
	sys_reset();

	while (1) {
		osDelay(1000);
	}
}

int update_ota_connect_server(update_cfg_local_t *cfg)
{
	struct sockaddr_in server_addr;
	int server_socket;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		printf("\n\r[%s] Create socket failed", __FUNCTION__);
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = cfg->ip_addr;
	server_addr.sin_port = cfg->port;

	if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		printf("\n\r[%s] Socket connect failed", __FUNCTION__);
		return -1;
	}

	return server_socket;
}

static void update_ota_local_task(void *param)
{
	int server_socket;
	uint8_t *buf;
	int read_bytes = 0;
	uint32_t idx = 0;
	update_cfg_local_t *cfg = (update_cfg_local_t *)param;
	uint32_t file_info[3];
	int ret = -1 ;
	uint8_t cur_fw_idx = 0, target_fw_idx = 0;
	uint32_t ota_len = 0;
	_file_checksum file_checksum;
	file_checksum.u = 0;
	uint32_t buf_size = 0;
	uint8_t boot_sel = -1;
	uint32_t total_blocks = 0, cur_block = 0;

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	rtw_create_secure_context(configMINIMAL_SECURE_STACK_SIZE);
#endif
	printf("\n\r[%s] Update task start", __FUNCTION__);

	boot_sel = sys_get_boot_sel();
	if (0 == boot_sel) {
		// boot from NOR flash
		buf_size = NOR_BLOCK_SIZE;
	} else if (1 == boot_sel) {
		// boot from NAND flash
		buf_size = NAND_BLOCK_SIZE;
	}

	buf = update_malloc(buf_size);
	if (!buf) {
		printf("\n\r[%s] Alloc buffer failed", __FUNCTION__);
		goto update_ota_exit;
	}

	// Connect server
	server_socket = update_ota_connect_server(cfg);
	if (server_socket == -1) {
		goto update_ota_exit;
	}

	// Get file size from DownloadServer.
	memset(file_info, 0, sizeof(file_info));
	if (file_info[0] == 0) {
		printf("\n\r[%s] Read info first", __FUNCTION__);
		read_bytes = read(server_socket, file_info, sizeof(file_info));
		if (read_bytes <= 0) {
			printf("\n\r[%s] Read socket failed or socket closed", __FUNCTION__);
			goto update_ota_exit;
		}
		// !X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X
		// !W checksum !W padding 0 !W file size !W
		// !X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X!X
		printf("\n\r[%s] info %d bytes", __FUNCTION__, read_bytes);
		printf("\n\r[%s] tx file size 0x%x", __FUNCTION__, file_info[2]);
		if (file_info[2] == 0) {
			printf("\n\r[%s] No file size", __FUNCTION__);
			goto update_ota_exit;
		} else {
			ota_len = file_info[2];
			total_blocks = (ota_len + buf_size - 1) / buf_size;
		}
	}

	cur_fw_idx = hal_sys_get_ld_fw_idx();
	printf("\n\r[%s] Current firmware index is %d\r\n", __FUNCTION__, cur_fw_idx);
	if (1 == cur_fw_idx) {
		target_fw_idx = 2;
	} else if (2 == cur_fw_idx) {
		target_fw_idx = 1;
	} else {
		goto update_ota_exit;
	}

	// Write New FW sector
	printf("\n\r[%s] Start to read data %d bytes\r\n", __FUNCTION__, ota_len);
	while (1) {
		int rest_len = ota_len - idx;
		int data_len = rest_len > buf_size ? buf_size : rest_len;

		memset(buf, 0, buf_size);
		read_bytes = 0;
		while (read_bytes < data_len) {
			read_bytes += read(server_socket, &buf[read_bytes], data_len - read_bytes);
			if (read_bytes < 0) {
				printf("\n\r[%s] Read socket failed", __FUNCTION__);
				goto update_ota_exit;
			}
		}
		if (read_bytes == 0) {
			break;    // Read end
		}

		printf("..");

		if ((idx + read_bytes) > ota_len) {
			printf("\n\r[%s] Redundant bytes received", __FUNCTION__);
			read_bytes = ota_len - idx;
			data_len = read_bytes;
		}

#if USE_CHECKSUM
		// checksum attached at file end
		if ((idx + read_bytes) > (ota_len - 4)) {
			file_checksum.c[0] = buf[read_bytes - 4];
			file_checksum.c[1] = buf[read_bytes - 3];
			file_checksum.c[2] = buf[read_bytes - 2];
			file_checksum.c[3] = buf[read_bytes - 1];
		}
#endif
		// check final block
		cur_block = idx / buf_size;
		if (cur_block == (total_blocks - 1)) {
			data_len -= 4; // remove final 4 bytes checksum
			memset(buf + data_len, 0xFF, buf_size - data_len);
		}

		if (0 == boot_sel) {
			ret = ota_flash_NOR(target_fw_idx, total_blocks, cur_block, buf, data_len, file_checksum);
		} else if (1 == boot_sel) {
			ret = ota_flash_NAND(target_fw_idx, total_blocks, cur_block, buf, data_len, file_checksum);
		}

		if (ret < 0) {
			printf("\n\r[%s] ota flash failed", __FUNCTION__);
			goto update_ota_exit;
		}

		idx += read_bytes;

		if (idx == ota_len) {
			break;
		}
	}
	printf("\n\rRead data finished\r\n");

update_ota_exit:
	if (buf) {
		update_free(buf);
	}
	if (server_socket >= 0) {
		close(server_socket);
	}
	if (param) {
		update_free(param);
	}
	TaskOTA = NULL;
	printf("\n\r[%s] Update task exit", __FUNCTION__);
	if (!ret) {
		printf("\n\r[%s] Ready to reboot", __FUNCTION__);
		osDelay(100);
		ota_platform_reset();
	}
	vTaskDelete(NULL);
	return;
}

int update_ota_local(char *ip, int port)
{
	update_cfg_local_t *pUpdateCfg;

	if (TaskOTA) {
		printf("\n\r[%s] Update task has created.", __FUNCTION__);
		return 0;
	}
	pUpdateCfg = update_malloc(sizeof(update_cfg_local_t));
	if (pUpdateCfg == NULL) {
		printf("\n\r[%s] Alloc update cfg failed", __FUNCTION__);
		return -1;
	}
	pUpdateCfg->ip_addr = inet_addr(ip);
	pUpdateCfg->port = ntohs(port);

	if (xTaskCreate(update_ota_local_task, "OTA_server", STACK_SIZE, pUpdateCfg, TASK_PRIORITY, &TaskOTA) != pdPASS) {
		update_free(pUpdateCfg);
		printf("\n\r[%s] Create update task failed", __FUNCTION__);
	}
	return 0;
}

void cmd_update(int argc, char **argv)
{
	int port;
	if (argc != 3) {
		printf("\n\r[%s] Usage: update IP PORT", __FUNCTION__);
		return;
	}
	port = atoi(argv[2]);
	update_ota_local(argv[1], port);
}

// choose the activated image. 0: default image / 1: upgrade image
void cmd_ota_image(bool cmd)
{
	if (cmd == 1) {
		sys_recover_ota_signature();
	} else {
		sys_clear_ota_signature();
	}
}

#ifdef HTTP_OTA_UPDATE
static char *redirect = NULL;
static int redirect_len;
static uint16_t redirect_server_port;
static char *redirect_server_host = NULL;
static char *redirect_resource = NULL;
int parser_url(char *url, char *host, uint16_t *port, char *resource)
{
	if (url) {
		char *http = NULL, *pos = NULL;

		http = strstr(url, "http://");
		if (http) { // remove http
			url += strlen("http://");
		}
		memset(host, 0, redirect_len);

		pos = strstr(url, ":");	// get port
		if (pos) {
			memcpy(host, url, (pos - url));
			pos += 1;
			*port = atoi(pos);
		} else {
			pos = strstr(url, "/");
			if (pos) {
				memcpy(host, url, (pos - url));
				url = pos;
			}
			*port = 80;
		}
		printf("server: %s\n\r", host);
		printf("port: %d\n\r", *port);

		memset(resource, 0, redirect_len);
		pos = strstr(url, "/");
		if (pos) {
			memcpy(resource, pos + 1, strlen(pos + 1));
		}
		printf("resource: %s\n\r", resource);

		return 0;
	}
	return -1;
}


/******************************************************************************************************************
** Function Name  : update_ota_http_connect_server
** Description    : connect to the OTA server
** Input          : server_socket: the socket used
**					host: host address of the OTA server
**					port: port of the OTA server
** Return         : connect ok:	socket value
**					Failed:		-1
*******************************************************************************************************************/
int update_ota_http_connect_server(int server_socket, char *host, int port)
{
	struct sockaddr_in server_addr;
	struct hostent *server;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		printf("\n\r[%s] Create socket failed", __FUNCTION__);
		return -1;
	}

	server = gethostbyname(host);
	if (server == NULL) {
		printf("[ERROR] Get host ip failed\n");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, 4);

	if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("\n\r[%s] Socket connect failed", __FUNCTION__);
		return -1;
	}

	return server_socket;
}

/******************************************************************************************************************
** Function Name  : parse_http_response
** Description    : Parse the http response to get some useful parameters
** Input          : response	: The http response got from server
**					response_len: The length of http response
**					result		: The struct that store the usful infor about the http response
** Return         : Parse OK:	1 -> Only got the status code
**								3 -> Got the status code and content_length, but didn't get the full header
**								4 -> Got all the information needed
**					Failed:		-1
*******************************************************************************************************************/
int parse_http_response(uint8_t *response, uint32_t response_len, http_response_result_t *result)
{
	uint32_t i, p, q, m;
	uint32_t header_end = 0;

	//Get status code
	if (0 == result->parse_status) { //didn't get the http response
		uint8_t status[4] = {0};
		i = p = q = m = 0;
		for (; i < response_len; ++i) {
			if (' ' == response[i]) {
				++m;
				if (1 == m) {//after HTTP/1.1
					p = i;
				} else if (2 == m) { //after status code
					q = i;
					break;
				}
			}
		}
		if (!p || !q || q - p != 4) { //Didn't get the status code
			return -1;
		}
		memcpy(status, response + p + 1, 3); //get the status code
		result->status_code = atoi((char const *)status);
		if (result->status_code == 200) {
			result->parse_status = 1;
		} else if (result->status_code == 302) {
			char *tmp = NULL;
			const uint8_t *location1 = (uint8_t *)"LOCATION";
			const uint8_t *location2 = (uint8_t *)"Location";
			printf("response 302:%s \r\n", response);

			if ((tmp = strstr((char const *)response, (char const *)location1)) || (tmp = strstr((char const *)response, (char const *)location2))) {
				redirect_len = strlen(tmp + 10);
				printf("Location len = %d\r\n", redirect_len);
				if (redirect == NULL) {
					redirect = update_malloc(redirect_len);
					if (redirect == NULL) {
						return -1;
					}
				}
				memset(redirect, 0, redirect_len);
				memcpy(redirect, tmp + 10, strlen(tmp + 10));
			}

			if (redirect_server_host == NULL) {
				redirect_server_host = update_malloc(redirect_len);
				if (redirect_server_host == NULL) {
					return -1;
				}
			}

			if (redirect_resource == NULL) {
				redirect_resource = update_malloc(redirect_len);
				if (redirect_resource == NULL) {
					return -1;
				}
			}

			memset(redirect_server_host, 0, redirect_len);
			memset(redirect_resource, 0, redirect_len);
			if (parser_url(redirect, redirect_server_host, &redirect_server_port, redirect_resource) < 0) {
				return -1;
			}
			return -1;
		} else {
			printf("\n\r[%s] The http response status code is %d", __FUNCTION__, result->status_code);
			return -1;
		}
	}

	//if didn't receive the full http header
	if (3 == result->parse_status) { //didn't get the http response
		p = q = 0;
		for (i = 0; i < response_len; ++i) {
			if (response[i] == '\r' && response[i + 1] == '\n' &&
				response[i + 2] == '\r' && response[i + 3] == '\n') { //the end of header
				header_end = i + 4;
				result->parse_status = 4;
				result->header_len = header_end;
				result->body = response + header_end;
				break;
			}
		}
		if (3 == result->parse_status) {//Still didn't receive the full header
			result->header_bak = update_malloc(HEADER_BAK_LEN + 1);
			memset(result->header_bak, 0, strlen((char const *)result->header_bak));
			memcpy(result->header_bak, response + response_len - HEADER_BAK_LEN, HEADER_BAK_LEN);
		}
	}

	//Get Content-Length
	if (1 == result->parse_status) { //didn't get the content length
		const uint8_t *content_length_buf1 = (uint8_t *)"CONTENT-LENGTH";
		const uint8_t *content_length_buf2 = (uint8_t *)"Content-Length";
		const uint32_t content_length_buf_len = strlen((char const *)content_length_buf1);
		p = q = 0;

		for (i = 0; i < response_len; ++i) {
			if (response[i] == '\r' && response[i + 1] == '\n') {
				q = i;//the end of the line
				if (!memcmp(response + p, content_length_buf1, content_length_buf_len) ||
					!memcmp(response + p, content_length_buf2, content_length_buf_len)) { //get the content length
					int j1 = p + content_length_buf_len, j2 = q - 1;
					while (j1 < q && (*(response + j1) == ':' || *(response + j1) == ' ')) {
						++j1;
					}
					while (j2 > j1 && *(response + j2) == ' ') {
						--j2;
					}
					uint8_t len_buf[12] = {0};
					memcpy(len_buf, response + j1, j2 - j1 + 1);
					result->body_len = atoi((char const *)len_buf);
					result->parse_status = 2;
				}
				p = i + 2;
			}
			if (response[i] == '\r' && response[i + 1] == '\n' &&
				response[i + 2] == '\r' && response[i + 3] == '\n') { //Get the end of header
				header_end = i + 4; //p is the start of the body
				if (result->parse_status == 2) { //get the full header and the content length
					result->parse_status = 4;
					result->header_len = header_end;
					result->body = response + header_end;
				} else { //there are no content length in header
					printf("\n\r[%s] No Content-Length in header", __FUNCTION__);
					return -1;
				}
				break;
			}
		}

		if (1 == result->parse_status) {//didn't get the content length and the full header
			result->header_bak = update_malloc(HEADER_BAK_LEN + 1);
			memset(result->header_bak, 0, strlen((char const *)result->header_bak));
			memcpy(result->header_bak, response + response_len - HEADER_BAK_LEN, HEADER_BAK_LEN);
		} else if (2 == result->parse_status) { //didn't get the full header but get the content length
			result->parse_status = 3;
			result->header_bak = update_malloc(HEADER_BAK_LEN + 1);
			memset(result->header_bak, 0, strlen((char const *)result->header_bak));
			memcpy(result->header_bak, response + response_len - HEADER_BAK_LEN, HEADER_BAK_LEN);
		}
	}

	return result->parse_status;
}

int http_update_ota(char *host, int port, char *resource)
{
	int server_socket = -1;
	uint8_t *buf = NULL, *request = NULL;
	int read_bytes = 0;
	int read_rtn = 0;
	int ret = -1;
	uint8_t cur_fw_idx = 0, target_fw_idx = 0;
	uint32_t ota_len = 0;
	http_response_result_t rsp_result = {0};
	_file_checksum file_checksum;
	file_checksum.u = 0;
	uint32_t buf_size = 0;
	uint8_t boot_sel = -1;
	uint32_t total_blocks = 0, cur_block = 0;
	uint32_t idx = 0;
	uint32_t data_len = 0;

restart_http_ota:
	redirect_server_port = 0;
	memset(&rsp_result, 0, sizeof(rsp_result));

	boot_sel = sys_get_boot_sel();
	if (0 == boot_sel) {
		// boot from NOR flash
		buf_size = NOR_BLOCK_SIZE;
	} else if (1 == boot_sel) {
		// boot from NAND flash
		buf_size = NAND_BLOCK_SIZE;
	}

	buf = update_malloc(buf_size);
	if (!buf) {
		printf("\n\r[%s] Alloc buffer failed", __FUNCTION__);
		goto update_ota_exit;
	}

	// Connect server
	server_socket = update_ota_http_connect_server(server_socket, host, port);
	if (server_socket == -1) {
		goto update_ota_exit;
	}

	printf("\n\r");

	//send http request
	request = (unsigned char *) update_malloc(strlen("GET /") + strlen(resource) + strlen(" HTTP/1.1\r\nHost: ")
			  + strlen(host) + strlen("\r\n\r\n") + 1);
	sprintf((char *)request, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", resource, host);

	ret = write(server_socket, request, strlen((char const *)request));
	if (ret < 0) {
		printf("\n\r[%s] Send HTTP request failed", __FUNCTION__);
		goto update_ota_exit;
	}

	while (3 >= rsp_result.parse_status) { //still read header
		if (0 == rsp_result.parse_status) { //didn't get the http response
			memset(buf, 0, buf_size);
			read_bytes = read(server_socket, buf, buf_size);
			if (read_bytes <= 0) {
				printf("\n\r[%s] Read socket failed", __FUNCTION__);
				goto update_ota_exit;
			}

			idx = read_bytes;

			if (parse_http_response(buf, read_bytes, &rsp_result) == -1) {
				goto update_ota_exit;
			}
		} else if ((1 == rsp_result.parse_status) || (3 == rsp_result.parse_status)) { //just get the status code
			memset(buf, 0, buf_size);
			memcpy(buf, rsp_result.header_bak, HEADER_BAK_LEN);
			update_free(rsp_result.header_bak);
			rsp_result.header_bak = NULL;

			read_bytes = read(server_socket, buf + HEADER_BAK_LEN, (buf_size - HEADER_BAK_LEN));
			if (read_bytes <= 0) {
				printf("\n\r[%s] Read socket failed", __FUNCTION__);
				goto update_ota_exit;
			}

			idx = read_bytes + HEADER_BAK_LEN;

			if (parse_http_response(buf, read_bytes + HEADER_BAK_LEN, &rsp_result) == -1) {
				goto update_ota_exit;
			}
		}
	}

	if (0 == rsp_result.body_len) {
		printf("\n\r[%s] New firmware size = 0 !", __FUNCTION__);
		goto update_ota_exit;
	} else {
		printf("\n\r[%s] Download new firmware begin, total size : %d\n\r", __FUNCTION__, rsp_result.body_len);
		ota_len = rsp_result.body_len;
		total_blocks = (ota_len + buf_size - 1) / buf_size;
	}

	cur_fw_idx = hal_sys_get_ld_fw_idx();
	printf("\n\r[%s] Current firmware index is %d\r\n", __FUNCTION__, cur_fw_idx);
	if (1 == cur_fw_idx) {
		target_fw_idx = 2;
	} else if (2 == cur_fw_idx) {
		target_fw_idx = 1;
	} else {
		goto update_ota_exit;
	}

	// read http response body
	read_bytes = idx - rsp_result.header_len;
	if (read_bytes > 0) {
		memcpy(buf, buf + rsp_result.header_len, read_bytes);
		memset(buf + read_bytes, 0, buf_size - read_bytes);
	} else {
		memset(buf, 0, buf_size);
		read_bytes = 0;
	}

	idx = 0;
	while (idx < ota_len) {
		printf("..");
		data_len = ota_len - idx;
		if (data_len > buf_size) {
			data_len = buf_size;
		}

		while (read_bytes < data_len) {
			read_rtn = read(server_socket, &buf[read_bytes], data_len - read_bytes);
			if (read_rtn <= 0) {
				printf("\n\r[%s] Read socket failed", __FUNCTION__);
				goto update_ota_exit;
			}
			read_bytes += read_rtn;
		}

		if ((idx + read_bytes) > ota_len) {
			printf("\n\r[%s] Redundant bytes received", __FUNCTION__);
			read_bytes = ota_len - idx;
			data_len = read_bytes;
		}

#if USE_CHECKSUM
		// checksum attached at file end
		if ((idx + read_bytes) > (ota_len - 4)) {
			file_checksum.c[0] = buf[read_bytes - 4];
			file_checksum.c[1] = buf[read_bytes - 3];
			file_checksum.c[2] = buf[read_bytes - 2];
			file_checksum.c[3] = buf[read_bytes - 1];
		}
#endif
		// check final block
		cur_block = idx / buf_size;
		if (cur_block == (total_blocks - 1)) {
			data_len -= 4; // remove final 4 bytes checksum
			memset(buf + data_len, 0xFF, buf_size - data_len);
		}

		if (0 == boot_sel) {
			ret = ota_flash_NOR(target_fw_idx, total_blocks, cur_block, buf, data_len, file_checksum);
		} else if (1 == boot_sel) {
			ret = ota_flash_NAND(target_fw_idx, total_blocks, cur_block, buf, data_len, file_checksum);
		}

		if (ret < 0) {
			printf("\n\r[%s] ota flash failed", __FUNCTION__);
			goto update_ota_exit;
		}

		idx += read_bytes;

		memset(buf, 0, buf_size);
		read_bytes = 0;
	}
	printf("\n\r[%s] Download new firmware %d bytes completed\n\r", __FUNCTION__, idx);


update_ota_exit:
	if (buf) {
		update_free(buf);
	}
	if (request) {
		update_free(request);
	}
	if (server_socket >= 0) {
		close(server_socket);
	}

	// redirect_server_port != 0 means there is redirect URL can be downloaded
	if (redirect_server_port != 0) {
		host = redirect_server_host;
		resource = redirect_resource;
		port = redirect_server_port;
		printf("\n\r[%s] OTA redirect host: %s, port: %d, resource: %s", __FUNCTION__, host, port, resource);
		goto restart_http_ota;
	}

	if (redirect) {
		update_free(redirect);
	}
	if (redirect_server_host) {
		update_free(redirect_server_host);
	}
	if (redirect_resource) {
		update_free(redirect_resource);
	}

	return ret;
}
#endif
