#ifndef _VIDEO_API_H_
#define _VIDEO_API_H_

#include <stdint.h>
#include <osdep_service.h>
#include "hal_video.h"

#define VIDEO_SET_RCPARAM		0x10
#define VIDEO_FORCE_IFRAME		0x11
#define VIDEO_BPS               0x12
#define VIDEO_GOP				0x13
//#define VIDEO_JPEG_SNAPSHOT     0x14
//#define VIDEO_YUV_OUTPUT        0x15
#define VIDEO_ISP_SET_RAWFMT    0x16
//#define VIDEO_OSD				0x17
#define VIDEO_PRINT_INFO        0x18
#define VIDEO_DEBUG             0x19

#define VIDEO_HEVC_OUTPUT       0x20
#define VIDEO_H264_OUTPUT       0x21
#define VIDEO_JPEG_OUTPUT       0x22
#define VIDEO_NV12_OUTPUT       0x23
#define VIDEO_RGB_OUTPUT        0x24
#define VIDEO_NV16_OUTPUT       0x25


typedef struct encode_rc_parm_s {
	unsigned int rcMode;
	unsigned int iQp;		// for fixed QP
	unsigned int pQp;		// for fixed QP
	unsigned int minQp;		// for CBR/VBR
	unsigned int minIQp;	// for CBR/VBR
	unsigned int maxQp;		// for CBR/VBR
} encode_rc_parm_t;

typedef struct encode_rc_adv_parm_s {
	unsigned int rc_adv_enable;
	unsigned int maxBps;		// for VBR
	unsigned int minBps;		// for VBR

	int intraQpDelta;
	int mbQpAdjustment;
	unsigned int mbQpAutoBoost;
} encode_rc_adv_parm_t;

typedef struct encode_roi_parm_s {
	unsigned int enable;
	unsigned int left;		// for fixed QP
	unsigned int right;		// for fixed QP
	unsigned int top;		// for CBR/VBR
	unsigned int bottom;	// for CBR/VBR
} encode_roi_parm_t;


typedef struct video_state_s {
	uint32_t timer_1;
	uint32_t timer_2;
	uint32_t drop_frame;
} video_state_t;

typedef struct isp_info_s {
	uint32_t sensor_width;
	uint32_t sensor_height;
	uint32_t sensor_fps;
	uint32_t osd_enable;
	uint32_t md_enable;
	uint32_t hdr_enable;
	uint32_t osd_buf_size;
	uint32_t md_buf_size;
} isp_info_t;

typedef struct video_param_s {
	uint32_t stream_id;
	uint32_t type;
	uint32_t resolution;
	uint32_t width;
	uint32_t height;
	uint32_t bps;
	uint32_t fps;
	uint32_t gop;
	uint32_t rc_mode;
	uint32_t jpeg_qlevel;
	uint32_t rotation;
	uint32_t out_buf_size;
	uint32_t out_rsvd_size;
	uint32_t direct_output;
	uint32_t use_static_addr;
} video_params_t;


int video_start(hal_video_adapter_t  *v_adp, int ch, int type);

int video_stop(hal_video_adapter_t  *v_adp, int ch, int type);

int video_ctrl(hal_video_adapter_t  *v_adp, int ch, int cmd, int arg);

int video_set_roi_region(int ch, int x, int y, int width, int height, int value);

hal_video_adapter_t *video_init(int iq_start_addr, int sensor_start_addr);

void *video_deinit(hal_video_adapter_t  *v_adp);

void video_set_isp_info(isp_info_t *info);

int video_buf_calc(int v1_enable, int v1_w, int v1_h, int v1_bps, int v1_shapshot,
				   int v2_enable, int v2_w, int v2_h, int v2_bps, int v2_shapshot,
				   int v3_enable, int v3_w, int v3_h, int v3_bps, int v3_shapshot,
				   int v4_enable, int v4_w, int v4_h);

int video_alloc_heap(hal_video_adapter_t *v_adp, int heap_size);

int video_open(hal_video_adapter_t  *v_adp, video_params_t *v_stream, output_callback_t output_cb, void *ctx);

int video_close(hal_video_adapter_t  *v_adp, int ch);

int video_encbuf_release(int ch, int codec, int length);

int video_ispbuf_release(int ch, int addr);

int isp_ctrl_cmd(int argc, char **argv);

int iq_tuning_cmd(int argc, char **argv);

int video_i2c_cmd(int argc, char **argv);

#endif

