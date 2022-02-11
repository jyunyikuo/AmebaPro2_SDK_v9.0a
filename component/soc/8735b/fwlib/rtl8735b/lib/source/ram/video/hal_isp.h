/*
 * hal_isp.h
 *
 *  Created on: 2021¦~3¤ë17¤é
 *      Author: martinhuang
 */

#ifndef HAL_RTL8735B_LIB_SOURCE_RAM_VIDEO_ISP_HAL_ISP_H_
#define HAL_RTL8735B_LIB_SOURCE_RAM_VIDEO_ISP_HAL_ISP_H_

#if !defined (CONFIG_VOE_PLATFORM) || !CONFIG_VOE_PLATFORM // Run on TM9
#include "cmsis.h"
#else
#include "cmsis_voe.h"
#include "voe.h"
#endif

#define TOTAL_STEAM_NUM 5
#define MAX_SW_BUFFER 5
//#define RTS_VIDEOIN_HEIGHT_ALIGN	16

typedef struct hal_isp_buffer {
	int	state;
	uint32_t buf_addr;
	uint32_t org_buf_addr;
} hal_isp_buffer_t;


enum ameba_stream_fmt {
	NV12_FORMAT,
	NV16_FORMAT,
	NV21_FORMAT,
	NV61_FORMAT,
	RGB888_FORMAT
};



typedef struct hal_isp_stream_stream {

	__u8 streamid;	/*initialized by user*/

	uint32_t fmt;
	uint32_t user_width;
	uint32_t user_height;

	uint32_t rate_numerator;
	uint32_t rate_denominator;
	int fnum;

	unsigned long frame_count;
	unsigned long skip_count;
	unsigned long overflow_count;
	unsigned long error_count;

	hal_isp_buffer_t bufs[MAX_SW_BUFFER];

	uint8_t hw_slot_num;	/*initialized by user*/
	uint8_t buff_num;
	uint8_t	bits_pixel;

	int buf_release_cnt[MAX_SW_BUFFER];
	int stream_running;

} hal_isp_stream_t;


typedef struct {


	hal_isp_stream_t video_stream[TOTAL_STEAM_NUM];


	uint16_t snr_width;
	uint16_t snr_height;

	uint16_t fr_width;
	uint16_t fr_height;

	uint16_t fr_cnt;
	uint8_t sub_module;
	uint8_t vhdr;

	uint32_t short_exp_line;

	uint16_t fps;
	uint8_t path;
	uint8_t fixp_sensor;

	uint8_t dec_format;
	uint8_t raw_source;
	uint8_t dec_id;
	uint8_t ssor_clock;

	uint32_t isp_enable;
	uint32_t vhdr_ctrl;
	uint32_t vhdr_ratio;
	uint32_t mipi_buffer;

	uint32_t t_line_blk; //ns

	uint32_t interface_clk;
	uint32_t pixel_clk;
	uint32_t isp_clk;

//    uint32_t *addr3dnr;
	uint32_t addr_md0;
	uint32_t addr_md1;

	uint32_t addr_statics;

	//u32 isp_start_init_time;
	//u32 isp_start_time;
	//u32 isp_end_time;

	void *v_adapter;

	int isp_init_done;
	int stream_condtion; // bit0: yuv_stream0, bit1: yuv_stream1, , bit2: yuv_stream2, bit4: rgb_stream,
	int isp_device_probe_done;

	int *iq_addr;		// Added By Raymond for load iq.bin

	uint8_t fcs_ready;


	uint32_t isp_init_start;
	uint32_t isp_init_end;
	uint32_t isp_open_start;
	uint32_t isp_open_end;
	uint32_t isp_start_start;
	uint32_t isp_start_end;

	uint32_t set_pwr_time;
	uint32_t set_i2c_time;
	uint32_t set_i2c_end_time;
	uint32_t sensor_start;
	uint32_t sensor_end;
	uint32_t first_frame_done;
	uint32_t voe_open_start;
	uint32_t set_frmival_time;
	uint32_t streamon_time;

	uint32_t raw_fmt;

//	uint32_t md_en;

	uint32_t sensor_rst_pin;
	uint32_t sensor_pwdn_pin;
	uint32_t sensor_pwrctrl_pin;

} __attribute__((aligned(32))) hal_isp_adapter_t;




void *isp_soc_start(hal_isp_adapter_t *isp_adpt);
int isp_open_stream(hal_isp_adapter_t *isp_adpt, uint8_t stream_id);
int isp_close_stream(hal_isp_adapter_t *isp_adpt, uint8_t stream_id);
int isp_get_stream_cnt(uint8_t stream_id);
uint32_t isp_get_frame_buffer(uint8_t stream_id);
int isp_release_frame_buffer(uint8_t stream_id, uint32_t buf_addr);
int isp_locate_buffer(hal_isp_adapter_t *isp_adpt, uint8_t stream_id);
int isp_free_buffer(hal_isp_adapter_t *isp_adpt, uint8_t stream_id);
int check_isp_soc_start_done(hal_isp_adapter_t *isp_adpt);
int hal_isp_init(hal_isp_adapter_t *isp_adpt);
//int isp_lock_rgb_buf_and_replace(hal_isp_adapter_t *isp_adpt, uint32_t lock_buf, uint8_t hw_slot);
//int isp_rgb_release_buffer(void);
int hal_isp_set_raw_fmt(uint8_t streamid, uint32_t fmt);

#endif /* HAL_RTL8735B_LIB_SOURCE_RAM_VIDEO_ISP_HAL_ISP_H_ */
