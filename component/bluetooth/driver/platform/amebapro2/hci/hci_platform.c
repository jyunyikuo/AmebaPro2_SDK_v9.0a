/*
 *******************************************************************************
 * Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 */

#include "osif.h"
#include "hci_config.h"
#include "hci/hci_common.h"
#include "hci_uart.h"
#include "hci_platform.h"
#include "hci_dbg.h"
#include "hal_api.h"
#include "device_lock.h"
#include "bt_intf.h"

#define HCI_LGC_EFUSE_LEN          0x30
#define HCI_PHY_EFUSE_LEN          0x30
#define HCI_PHY_EFUSE_BASE         0x510
#define HCI_LGC_EFUSE_OFFSET       0x190
#define HCI_MAC_ADDR_LEN           6
#define HCI_CONFIG_SIGNATURE       0x8723ab55
#define HCI_CONFIG_HDR_LEN         6
#define HCI_PATCH_FRAG_SIZE        252
#define HCI_PATCH_FLASH_ADDRESS    0x080F8000

#define HCI_CFG_BAUDRATE           BIT0
#define HCI_CFG_FLOWCONTROL        BIT1
#define HCI_CFG_BD_ADDR            BIT2
#define LEFUSE(x)                  ((x)-HCI_LGC_EFUSE_OFFSET)

uint32_t hci_cfg_sw_val = 0xFF;    // Open BT Trace log & FW log use 0xDD

extern const unsigned char rtlbt_fw[];
extern unsigned int rtlbt_fw_len;
extern const unsigned char rtlbt_mp_fw[];
extern unsigned int rtlbt_mp_fw_len;
//static const uint8_t hci_patch_buf[] = {0xff, 0xff, 0xff, 0xff};
//static uint32_t hci_patch_buf_len    = sizeof(hci_patch_buf);
static uint8_t hci_phy_efuse[HCI_PHY_EFUSE_LEN]  = {0};
static uint8_t hci_lgc_efuse[HCI_LGC_EFUSE_LEN]  = {0};
static uint8_t hci_chipid_in_fw  = 0;
static uint8_t hci_init_config[] = {
	0x55, 0xab, 0x23, 0x87,

	0x19, 0x00,
	//0x10, 0x00,
	0x30, 0x00, 0x06, 0x11, 0x28, 0x36, 0x12, 0x51, 0x89, /* BT MAC address */
	//	0x07, 0x00,
	0x08, 0x00, 0x04, 0x00, 0xC2, 0x01, 0x00,/* Log Baudrate 115200 */
#ifdef CONFIG_MP_INCLUDED
	0x0c, 0x00, 0x04, 0x1d, 0x70, 0x00, 0x00,/* Baudrate 115200 */
#else
	0x0c, 0x00, 0x04, 0x04, 0x50, 0xF7, 0x03, /* Baudrate 921600 */
	0x18, 0x00, 0x01, 0x5c, /* flow control */
#endif
	//efuse value
	0x78, 0x02, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x07,
	0x83, 0x02, 0x0A, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x64,
};
typedef struct {
	uint8_t *fw_buf;
	uint8_t fw_is_alloced;
	uint16_t fw_len;
	uint8_t *config_buf;
	uint8_t cfg_is_alloced;
	uint16_t config_len;
	uint8_t cur_index;
	uint8_t end_index;
	uint8_t last_pkt;
	uint32_t sent_len;
} HCI_PATCH_INFO;
static HCI_PATCH_INFO *hci_patch_info = NULL;

static uint8_t  hci_cfg_bd_addr[HCI_MAC_ADDR_LEN] = {0};
static uint32_t hci_cfg_log_uart_baudrate         = 115200;
static uint8_t  hci_cfg_log_bt_baudrate[4]        = {0x1d, 0x70, 0x00, 0x00};
static uint32_t hci_cfg_init_uart_baudrate        = 115200;
static uint32_t hci_cfg_work_uart_baudrate        = 921600;
static uint8_t  hci_cfg_work_bt_baudrate[4]       = {0x04, 0x50, 0xF7, 0x03};
static uint32_t hci_cfg_flag                      = 0;

extern void rltk_coex_bt_enable(u8 enable);

static uint32_t cal_bit_shift(uint32_t Mask)
{
	uint32_t i;
	for (i = 0; i < 31; i++) {
		if (((Mask >> i) & 0x1) == 1) {
			break;
		}
	}
	return (i);
}

static void set_reg_value(uint32_t reg_address, uint32_t Mask, uint32_t val)
{
	uint32_t shift = 0;
	uint32_t data = 0;
	data = HAL_READ32(reg_address, 0);
	shift = cal_bit_shift(Mask);
	data = ((data & (~Mask)) | (val << shift));
	HAL_WRITE32(reg_address, 0, data);
	data = HAL_READ32(reg_address, 0);
}

void hci_platform_cfg_bd_addr(uint8_t *bdaddr)
{
	for (uint8_t i = 0; i < HCI_MAC_ADDR_LEN; i++) {
		hci_cfg_bd_addr[i] = bdaddr[i];
	}

	hci_cfg_flag |= HCI_CFG_BD_ADDR;
}

static void hci_platform_convert_baudrate(uint32_t *bt_baudrate, uint32_t *uart_baudrate, uint8_t bt_to_uart)
{
	uint8_t i;

	const struct {
		uint32_t bt_baudrate;
		uint32_t uart_baudrate;
	} baudrate_map[] = {
		{0x0000701d, 115200},
		{0x0252C00A, 230400},
		{0x03F75004, 921600},
		{0x05F75004, 921600},
		{0x00005004, 1000000},
		{0x04928002, 1500000},
		{0x00005002, 2000000},
		{0x0000B001, 2500000},
		{0x04928001, 3000000},
		{0x052A6001, 3500000},
		{0x00005001, 4000000},
	};

	const uint32_t baudrate_map_len = sizeof(baudrate_map) / sizeof(baudrate_map[0]);

	if (bt_to_uart) {
		for (i = 0; i < baudrate_map_len; i++) {
			if (*bt_baudrate == baudrate_map[i].bt_baudrate) {
				break;
			}
		}

		if (i == baudrate_map_len) {
			HCI_ERR("Wrong Baudrate Selection! Use Default 115200!");
			i = 0;
		}
		*uart_baudrate = baudrate_map[i].uart_baudrate;
	} else {
		for (i = 0; i < baudrate_map_len; i++) {
			if (*uart_baudrate == baudrate_map[i].uart_baudrate) {
				break;
			}
		}

		if (i == baudrate_map_len) {
			HCI_ERR("Wrong Baudrate Selection! Use Default 115200!");
			i = 0;
		}
		*bt_baudrate = baudrate_map[i].bt_baudrate;
	}
}

static uint8_t hci_platform_read_efuse(void)
{
	//phy_efuse
	// hci_board_debug("\n phy_efuse data end  is =============\n");
	for (int i = 0; i < HCI_PHY_EFUSE_LEN; i++) {
		hci_phy_efuse[i] = otp_byte_read(HCI_PHY_EFUSE_BASE + i);
	}

	//logic_efuse
	otp_logical_read(HCI_LGC_EFUSE_OFFSET, HCI_LGC_EFUSE_LEN, hci_lgc_efuse);
#if 0

	printf("\r\n==bt phy_efuse data is:==\r\n ");
	for (int i = 0; i < HCI_PHY_EFUSE_LEN; i++) {
		printf("%x:", hci_phy_efuse[i]);
	}
	printf("\n lgc efuse data is =============\n");
	for (int i = 0; i < HCI_LGC_EFUSE_LEN; i++) {
		printf("%x:", hci_lgc_efuse[i]);
	}
	printf("\n efuse data end	is =============\n");

#endif
	return true;
}

static uint8_t hci_platform_parse_config(void)
{
	uint8_t *p, i;
	uint16_t entry_offset, entry_len;
	//uint16_t tx_flatk;

	if (sizeof(hci_init_config) <= HCI_CONFIG_HDR_LEN) {
		return HCI_IGNORE;
	}

	p = hci_init_config;
	if (HCI_CONFIG_SIGNATURE != *(uint32_t *)(p)) {
		return HCI_FAIL;
	}

	if (*(uint16_t *)(p + 4) != (uint16_t)(sizeof(hci_init_config) - HCI_CONFIG_HDR_LEN)) {
		/* Fix the len, just avoid the length is not correct */
		*(uint16_t *)(p + 4) = (uint16_t)(sizeof(hci_init_config) - HCI_CONFIG_HDR_LEN);
	}

	p += HCI_CONFIG_HDR_LEN;
	while (p < hci_init_config + sizeof(hci_init_config)) {
		entry_offset = *(uint16_t *)(p);
		entry_len = *(uint8_t *)(p + 2);
		p += 3;

		switch (entry_offset) {
		case 0x000c:
			/* MP Mode, Use Default: 115200 */
			if ((wifi_driver_is_mp()) || (!CHECK_CFG_SW(EFUSE_SW_UPPERSTACK_SWITCH))) {
				hci_platform_convert_baudrate((uint32_t *)p, &hci_cfg_init_uart_baudrate, 0);
			}

			hci_platform_convert_baudrate((uint32_t *)p, &hci_cfg_work_uart_baudrate, 1);
			hci_platform_convert_baudrate((uint32_t *)hci_cfg_work_bt_baudrate, &hci_cfg_work_uart_baudrate, 0);
			/* TODO: Config BaudRate */
			break;
		case 0x0018:
			/* MP Mode, Close Flow Control */
			if ((wifi_driver_is_mp()) || (!CHECK_CFG_SW(EFUSE_SW_UPPERSTACK_SWITCH))) {
				p[0] = p[0] & (~BIT2);
			}
			/* TODO: Config Flow Control */
			break;
		case 0x0030:
			/* Set ConfigBuf MacAddr, Use Customer Assign or Efuse */
			if (hci_cfg_flag & HCI_CFG_BD_ADDR) {
				for (i = 0; i < HCI_MAC_ADDR_LEN; i++) {
					p[i] = hci_cfg_bd_addr[i];
				}
			}
			else {
				if ((hci_lgc_efuse[LEFUSE(0x194)] != 0xff) || (hci_lgc_efuse[LEFUSE(0x195)] != 0xff) || (hci_lgc_efuse[LEFUSE(0x196)] != 0xff) || \
					(hci_lgc_efuse[LEFUSE(0x197)] != 0xff) || (hci_lgc_efuse[LEFUSE(0x198)] != 0xff) || (hci_lgc_efuse[LEFUSE(0x199)] != 0xff)) {
					for (i = 0; i < HCI_MAC_ADDR_LEN; i++) {
						p[i] = hci_lgc_efuse[LEFUSE(0x194) + HCI_MAC_ADDR_LEN - 1 - i];
					}
				}
			}
			break;
		case 0x0278:
			if (hci_lgc_efuse[LEFUSE(0x19e)] == 0xff) {
				if (!(hci_phy_efuse[2] & BIT0)) {
					//tx_flatk = hci_phy_efuse[0x0a] | hci_phy_efuse[0x0b] << 8;
					//bt_flatk_8730d(tx_flatk);
				}
				break;
			} else {
				p[0] = hci_lgc_efuse[LEFUSE(0x19e)];
				if (hci_lgc_efuse[LEFUSE(0x19e)] & BIT1) {
					p[1] = hci_lgc_efuse[LEFUSE(0x19f)];
				}

				if (hci_lgc_efuse[LEFUSE(0x19e)] & BIT2) {
					p[2] = hci_lgc_efuse[LEFUSE(0x1a0)];
					p[3] = hci_lgc_efuse[LEFUSE(0x1a1)];
					//tx_flatk = hci_lgc_efuse[LEFUSE(0x1a0)] | hci_lgc_efuse[LEFUSE(0x1a1)] << 8;
					//bt_flatk_8730d(tx_flatk);
				} else {
					if (!(hci_phy_efuse[2] & BIT0)) {
						//tx_flatk = hci_phy_efuse[0xa] | hci_phy_efuse[0xb] << 8;
						//bt_flatk_8730d(tx_flatk);
					}
				}

				if (hci_lgc_efuse[LEFUSE(0x19e)] & BIT5) {
					p[4] = hci_lgc_efuse[LEFUSE(0x1a2)];
					p[5] = hci_lgc_efuse[LEFUSE(0x1a3)];
				}
			}
			break;
		case 0x0283:
			for (i = 0; i < entry_len; i++) {
				if (hci_lgc_efuse[LEFUSE(0x1a4 + i)] != 0xff) {
					p[i] = hci_lgc_efuse[LEFUSE(0x1a4 + i)];
				}
			}
			break;
		default:
			break;
		}

		p +=  entry_len;
	}

	return HCI_SUCCESS;
}

#if defined(CONFIG_BUILD_NONSECURE)
extern void bt_power_on(void);
#else
static void bt_power_on(void)
{
	set_reg_value(0x50000848, BIT14, 1);
	osif_delay(5);
	set_reg_value(0x5000092C, BIT28, 1);
	osif_delay(5);
	HAL_WRITE32(0x40009830, 0, 0x7);
}
#endif

static void bt_power_off(void)
{
	HAL_WRITE32(0x40009830, 0, 0);
}

static void hci_platform_controller_reset(void)
{
	if (!CHECK_CFG_SW(EFUSE_SW_BT_FW_LOG)) {
		HCI_INFO("FW LOG OPEN");
		/* Open BT FW Log */
		set_reg_value(0x40000048, BIT0 | BIT1 | BIT2 | BIT3, 7);
		osif_delay(5);
		set_reg_value(0x40009090, BIT4, 0);
		osif_delay(5);
	}

	/* BT Controller Power */
	bt_power_on();
	osif_delay(5);
	
	HCI_INFO("BT Reset OK!");
}

uint8_t hci_platform_init(void)
{
	if (!(wifi_is_running(WLAN0_IDX) || wifi_is_running(WLAN1_IDX))) {
		HCI_ERR("Wifi is OFF! Restart Wifi First!");
		return HCI_FAIL;
	}

	if (!wifi_driver_is_mp()) {
		wifi_set_powersave_mode(IPS_MODE_NONE, LPS_MODE_NONE);
	}

	/* Read Efuse and Parse Configbuf */
	if (HCI_FAIL == hci_platform_read_efuse()) {
		return HCI_FAIL;
	}

	if (HCI_FAIL == hci_platform_parse_config()) {
		return HCI_FAIL;
	}

	/* BT Controller Reset */
	hci_platform_controller_reset();

	/* UART Init */
	if (HCI_FAIL == hci_uart_open()) {
		return HCI_FAIL;
	}

	/* Coex: TODO */
	if (wifi_driver_is_mp()) {
		/* Set GNT BT */
		rtlk_bt_set_gnt_bt(PTA_BT);
	} else {
		rltk_coex_bt_enable(1);
	}

	return HCI_SUCCESS;
}

uint8_t hci_platform_deinit(void)
{
	/* BT Controller Power Off */
	bt_power_off();

	/* UART Deinit */
	hci_uart_close();

	/* Coex */
	if (!wifi_driver_is_mp()) {
		rltk_coex_bt_enable(0);
	}

	return HCI_SUCCESS;
}

void hci_platform_record_chipid(uint8_t chipid)
{
	hci_chipid_in_fw = chipid;
}

void hci_platform_get_baudrate(uint8_t *baudrate, uint8_t len)
{
	/* memcpy */
	for (uint8_t i = 0; i < len; i++) {
		baudrate[i] = hci_cfg_work_bt_baudrate[i];
	}
}

uint8_t hci_platform_set_baudrate(void)
{
	hci_uart_set_bdrate(hci_cfg_work_uart_baudrate);
	osif_delay(10);

	return HCI_SUCCESS;
}

uint8_t hci_platform_dl_patch_init(void)
{
	hci_patch_info = osif_mem_alloc(0, sizeof(HCI_PATCH_INFO));
	if (!hci_patch_info) {
		return HCI_FAIL;
	}

	memset(hci_patch_info, 0, sizeof(HCI_PATCH_INFO));

	return HCI_SUCCESS;
}

void hci_platform_dl_patch_done(void)
{
	if (hci_patch_info->fw_is_alloced && hci_patch_info->fw_buf) {
		osif_mem_free(hci_patch_info->fw_buf);
	}
	hci_patch_info->fw_buf = NULL;

	if (hci_patch_info) {
		osif_mem_free(hci_patch_info);
	}
	hci_patch_info = NULL;
}

static inline void hci_platform_flash_stream_read(uint8_t *address, uint32_t len, uint8_t *data)
{
	memcpy(data, (const void *)address, len);
}

static uint8_t hci_platform_get_patch_info(void)
{
	const uint8_t   no_patch_sig[]     = {0xFF, 0xFF, 0xFF, 0xFF};
	const uint8_t   merged_patch_sig[] = {0x52, 0x65, 0x61, 0x6C, 0x74, 0x65, 0x63, 0x68};
	HCI_PATCH_INFO *patch_info         = hci_patch_info;
	uint16_t        num_of_patch, fw_chip_id, fw_len, i;
	uint32_t        fw_offset;

	if (CHECK_CFG_SW(EFUSE_SW_USE_FLASH_PATCH)) {
		if (wifi_driver_is_mp()) {
			patch_info->fw_buf = (uint8_t *)(void *)rtlbt_mp_fw;
			patch_info->fw_len = rtlbt_mp_fw_len;
		} else {		
			patch_info->fw_buf = (uint8_t *)(void *)rtlbt_fw;
			patch_info->fw_len = rtlbt_fw_len;
		}		
	} else {
		patch_info->fw_buf = (uint8_t *)HCI_PATCH_FLASH_ADDRESS;
	}

	if (!memcmp(patch_info->fw_buf, no_patch_sig, sizeof(no_patch_sig))) {
		return HCI_IGNORE;
	}

	if (!memcmp(patch_info->fw_buf, merged_patch_sig, sizeof(no_patch_sig))) {
		/* Merged Patch */
		hci_platform_flash_stream_read(patch_info->fw_buf + 0x0c, 2, (uint8_t *)&num_of_patch);

		for (i = 0; i < num_of_patch; i++) {
			hci_platform_flash_stream_read(patch_info->fw_buf + 0x0e + 2 * i, 2, (uint8_t *)&fw_chip_id);
			if (fw_chip_id == hci_chipid_in_fw) {
				hci_platform_flash_stream_read(patch_info->fw_buf + 0x0e + 2 * num_of_patch + 2 * i, 2, (uint8_t *)&fw_len);
				hci_platform_flash_stream_read(patch_info->fw_buf + 0x0e + 4 * num_of_patch + 4 * i, 4, (uint8_t *)&fw_offset);
				break;
			}
		}

		if (i >= num_of_patch) {
			return HCI_FAIL;
		}
		patch_info->fw_buf = patch_info->fw_buf + fw_offset;
		patch_info->fw_len = fw_len;
		patch_info->fw_is_alloced = 0;
	} else {
		/* Single Patch, Do Nothing */
		patch_info->fw_is_alloced = 0;
	}

	patch_info->config_buf = hci_init_config;
	patch_info->config_len = sizeof(hci_init_config);

	/* Calculate patch info */
	patch_info->end_index = (patch_info->fw_len + patch_info->config_len - 1) / HCI_PATCH_FRAG_SIZE;
	patch_info->last_pkt = (patch_info->fw_len + patch_info->config_len) % HCI_PATCH_FRAG_SIZE;
	if (patch_info->last_pkt == 0) {
		patch_info->last_pkt = HCI_PATCH_FRAG_SIZE;
	}

	return HCI_SUCCESS;
}

uint8_t hci_platform_get_patch_cmd_len(uint8_t *cmd_len)
{
	uint8_t ret;
	HCI_PATCH_INFO *patch_info = hci_patch_info;

	/* Download FW partial patch first time, get patch and info */
	if (0 == patch_info->cur_index) {
		ret = hci_platform_get_patch_info();
		if (HCI_SUCCESS != ret) {
			return ret;
		}
	}

	if (patch_info->cur_index == patch_info->end_index) {
		*cmd_len = patch_info->last_pkt + 1;
		return HCI_SUCCESS;
	}

	*cmd_len = HCI_PATCH_FRAG_SIZE + 1;

	return HCI_SUCCESS;
}

uint8_t hci_platform_get_patch_cmd_buf(uint8_t *cmd_buf, uint8_t cmd_len)
{
	HCI_PATCH_INFO *patch_info = hci_patch_info;
	uint8_t        *data_buf   = &cmd_buf[1];
	uint8_t         data_len   = cmd_len - 1;

	/* first byte is index */
	cmd_buf[0] = patch_info->cur_index % 0x80;
	if (patch_info->cur_index == patch_info->end_index) {
		cmd_buf[0] |= 0x80;
	}
	if (patch_info->sent_len + data_len <= patch_info->fw_len) {
		/* within fw patch domain */
		memcpy(data_buf, patch_info->fw_buf + patch_info->sent_len, data_len);
	} else if ((patch_info->sent_len < patch_info->fw_len) && (patch_info->sent_len + data_len > patch_info->fw_len)) {
		/* need copy fw patch domain and config domain */
		memcpy(data_buf, patch_info->fw_buf + patch_info->sent_len, patch_info->fw_len - patch_info->sent_len);

		memcpy(data_buf + (patch_info->fw_len - patch_info->sent_len), patch_info->config_buf,
			   data_len - (patch_info->fw_len - patch_info->sent_len));
	} else {
		memcpy(data_buf, patch_info->config_buf + (patch_info->sent_len - patch_info->fw_len), data_len);
	}

	patch_info->sent_len += data_len;
	patch_info->cur_index++;

	return HCI_SUCCESS;
}
