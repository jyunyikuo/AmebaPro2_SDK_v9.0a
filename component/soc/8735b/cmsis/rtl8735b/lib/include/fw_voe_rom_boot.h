/**************************************************************************//**
 * @file     fw_voe_rom_boot.h
 * @brief    Define the data structure for VOE ROM Boot functions.
 * @version  V1.00
 * @date     2021-11-15
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2016 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

#ifndef _FW_VOE_ROM_BOOT_H_
#define _FW_VOE_ROM_BOOT_H_
#include "cmsis.h"

#ifdef  __cplusplus
extern "C"
{
#endif


//#define _FPGA_MIPI_APHY_
//#define _USE_RAM_SIM_FCS_


#define ISP_MULTI_FCS_MAGIC_NUM 0x5343464D /* 'M', 'F', 'C', 'S' */
#define ISP_MFCS_DATA_VERSION   0x00010001    /*  Major-[31:16] */
#define MULTI_FCS_MAX           10


#define ISP_FCS_DATA_MAGIC_NUM  0x53434649 /* 'I', 'F', 'C', 'S' */
#define ISP_FCS_DATA_VERSION    0x00010000    /*  Major-[31:16] */
#define ISP_FCS_VERION_MJR_MASK 0xFFFF0000

#define GPIO_MAX                12
#define I2C_MAX                 2

#define ISP_FCS_OK              0x0


#define FCS_DATA_LOAD_OK_TM     0x0001
#define FULL_LOAD_OK_FCS_OK_TM  0x0002
#define FULL_LOAD_OK_FCS_NG_TM  0x0003
#define FULL_LOAD_OK_FCS_NK_TM  0x0004

#define FCS_INIT_ROM_RDY_KM     0x0080
#define FCS_WAIT_SNR_CLK_KM     0x0081
#define FCS_RUN_DATA_OK_KM      0x0082
#define FCS_RUN_DATA_NG_KM      0x2081



#define TM_ERROR                0x1000
#define FCS_DATA_HEADER_ERR     0x1001
#define FCS_DATA_GPIO_CNT_ERR   0x1002
#define FCS_CPY_FUNC_ERR        0x1003
#define FCS_ITCM_INFO_ERR       0x1004
#define FCS_DTCM_INFO_ERR       0x1005
#define MFCS_DATA_HEADER_ERR    0x1006
#define MFCS_FCS_CNT_ID_ERR     0x1007
#define MFCS_FCS_ADDR_SZ_ERR    0x1008
#define FCS_WAIT_KM_TMOUT_ERR   0x1009
#define FCS_WAIT_SNR_TMOUT_ERR  0x100A
#define FCS_INVALID_CMD_ERR     0x100B

#define KM_ERROR                0x2000
#define FCS_CMD_INVALID_ERR     0x2006
#define FCS_ERR_CMDID           0x2007
#define FCS_SNR_CLK_TYPE_ERR    0x2008
#define FCS_GPIO_INIT_ERR       0x2009
#define FCS_I2C_INIT_ERR        0x200A
#define FCS_ADC_INIT_ERR        0x200B
#define FCS_PWM_INIT_ERR        0x200C
#define FCS_DATA_I2C_CNT_ERR    0x200D

#define FCS_ADC_NIDLE_ERR       0x2010
#define FCS_ADC_INPROG_ERR      0x2011
#define FCS_ADC_SINREAD_ERR     0x2012
#define FCS_ADC_CALIPARA_ERR    0x2013
#define FCS_ADC_DIVBYZERO_ERR   0x2014
#define FCS_ADC_NEGVALUE_ERR    0x2015
#define FCS_ADC_OVERFLOW_ERR    0x2016


#define FCS_I2C_TRANS_ERR       0x2100
#define FCS_I2C_CMP_ERR         0x21FF
#define FCS_I2C_BSTWRITE_ERR    0x21FE
#define FCS_I2C_CB_ERR          0x21FD

#define KM_STATUS               0x40492004
#define KM_FCS_ERROR_REG        0x40492008

#define TM_STATUS               0x40009154

#define REMAP_S7_ITCM 0x04000000   //0x04000000~0x0407FFFF
#define REMAP_S7_DTCM 0x00080000   //0x20080000~0x200FFFFF

#define VOE_IROM_S 0x00000000
#define VOE_IROM_E 0x000003FF       // 1KB
#define VOE_IRAM_S 0x00040000
#define VOE_IRAM_E 0x0004FFFF       // 64KB

#define VOE_DROM_S 0x20010000
#define VOE_DROM_E 0x20013FFF       // 16KB
#define VOE_DRAM_S 0x20000000
#define VOE_DRAM_E 0x2000FFFF       // 64KB

//#define DDR_FOR_VOE_IMG_LOAD_ADDR             (0x70000000)

//#define WAIT_KM_INIT_TIMEOUT_US       5000

#define FCS_ADC_PARAM_IDX_MAX               (4)
#define FCS_ADC_PARAM_GAIN_DENO_SIZE        (2)
#define FCS_ADC_PARAM_GAIN_MOLE_SIZE        (2)
#define FCS_ADC_PARAM_OFFSET_DENO_SIZE      (2)
#define FCS_ADC_PARAM_OFFSET_MOLE_SIZE      (4)

typedef struct fcs_adc_paras_list_s {
	uint16_t otp_addr;
	uint16_t adc_para_size;
	uint8_t *p_adc_para_v;
} fcs_adc_paras_list_t, *pfcs_adc_paras_list_t;

enum {
	FCS_ADC_PARAM_GAIN_DENO_IDX        = 0x0,
	FCS_ADC_PARAM_GAIN_MOLE_IDX        = 0x1,
	FCS_ADC_PARAM_OFFSET_DENO_IDX      = 0x2,
	FCS_ADC_PARAM_OFFSET_MOLE_IDX      = 0x3,
};

typedef struct isp_multi_fcs_hdr_s {
	uint32_t magic;
	uint32_t version;
	uint32_t fcs_data_size[MULTI_FCS_MAX];
	uint32_t fcs_data_offset[MULTI_FCS_MAX];
	uint32_t wait_km_init_timeout_us;
	uint8_t  multi_fcs_cnt;
	uint8_t  active_fcs;
	uint8_t  reserved[6];
} isp_multi_fcs_hdr_t;

typedef struct isp_multi_fcs_info_s {
	uint32_t sensor_set_size[MULTI_FCS_MAX];
	uint32_t resv[22];
} isp_multi_fcs_info_t;

typedef struct iq_info_s {
	uint32_t start_addr;
	uint32_t data_size;
	uint32_t resv[14];
} iq_info_t;

typedef struct sensor_info_s {
	uint32_t start_addr;
	uint32_t data_size;
	uint32_t resv[14];
} sensor_info_t;

typedef struct sensor_set_info_s {
	uint32_t fcs_data_size;
	uint32_t fcs_data_offset;
	uint32_t iq_start_addr;
	uint32_t iq_data_size;
	uint32_t sensor_start_addr;
	uint32_t sensor_data_size;
	uint32_t resv[2];
} sensor_set_info_t;

typedef struct isp_multi_fcs_ld_info_s {
	uint8_t fcs_id;
	uint8_t multi_fcs_cnt;
	uint8_t resv1[2];
	uint32_t magic;
	uint32_t version;
	uint32_t wait_km_init_timeout_us;
	uint32_t fcs_hdr_start;
	uint32_t resv2[3];
	sensor_set_info_t sensor_set[MULTI_FCS_MAX];
} isp_multi_fcs_ld_info_t;

typedef struct isp_fcs_header_s {

	//struct isp_fcs_cmd_attribute_t cmd_attb;
	uint32_t magic;
	uint32_t version;
	uint32_t itcm_size;
	uint32_t itcm_addr;
	uint32_t dtcm_size;
	uint32_t dtcm_addr;


	uint8_t i2c_device_cnt;
	uint8_t i2c_speed_mode;
	uint16_t i2c_speed;     // unit: KHz

	uint16_t i2c_timeout;   //unit: us
	uint8_t i2c_id;
	uint8_t timer_id;

	uint8_t adc_id;   // use 1 gpio
	uint8_t pwm_id;   // same function as ir_led
	uint8_t gpio_list[GPIO_MAX];
	uint8_t gpio_cnt;


	uint8_t snr_clk_pin; // dedicated gpio
	uint8_t i2c_scl; // dedicated gpio
	uint8_t i2c_sda; // dedicated gpio
	uint8_t reserved0;

	uint8_t adc_th_num;
	uint8_t adc_th_table_num;
	uint8_t adc_value_byte;
	uint8_t adc_use_cali;

	int16_t adc_gain_deno;
	int16_t adc_gain_mole;
	int32_t adc_offset_mole;


	int16_t adc_offset_deno;
	int16_t wait_i2c_delay_us;
	uint32_t snr_timeout_us;

	uint16_t adc_trig_delay;
	uint8_t reserved[6];



} isp_fcs_header_t;

typedef struct isp_i2c_info_s {
	uint16_t i2c_slave_addr;
	uint8_t addr_len;
	uint8_t data_len;
} isp_i2c_info_t;

#if 0
typedef struct voe_header_s {
	u16 year;               // 2B
	u8 month;               // 1B
	u8 day;                 // 1B
	u8 version[24];         // 24B
	u32 itcm_size;          // 4B
	u32 dtcm_size;
	u32 ddr_size;
} voe_header_t;
#endif

typedef void (*voe_cpy_t)(void *, void *, size_t);
int hal_rtl_voe_fcs_process(voe_cpy_t voe_cpy, uint32_t mfcs_data_addr, uint8_t fcs_id, uint32_t total_size);
;

#ifdef  __cplusplus
}
#endif

#endif  // end of "#define _FW_UART_BOOT_H_"

