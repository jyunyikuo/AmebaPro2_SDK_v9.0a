/******************************************************************************
*
* Copyright(c) 2021 - 2025 Realtek Corporation. All rights reserved.
*
******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <osdep_service.h>
#include "mmf2.h"
#include "mmf2_dbg.h"

#include "module_video.h"
#include "module_rtsp2.h"


#include <math.h>
#include "platform_stdlib.h"

#include <unistd.h>
#include <sys/wait.h>

#include "base_type.h"

#include "cmsis.h"
#include "error.h"


#include "hal.h"
#include "hal_video.h"
#include "md/md2_api.h"

#define OSD_ENABLE 1
#define MD_ENABLE  1
#define HDR_ENABLE 1

int framecnt = 0;
int jpegcnt = 0;
int incb = 0;
int ch1framecnt = 0;
int ch2framecnt = 0;
int rgb_lock = 0;

#define VIDEO_DEBUG 0

void md_output_cb(void *param1, void  *param2, uint32_t arg)
{
#if MD_ENABLE
	md2_result_t *md_result = (md2_result_t *)param1;
	static int md_counter = 0;
	if (md_get_enable()) {
		printf("md_result: %d\n\r", md_result->motion_cnt);

		hal_video_md_trigger();
	}
#endif
}

void video_frame_complete_cb(void *param1, void  *param2, uint32_t arg)
{
	incb = 1;
	enc2out_t *enc2out = (enc2out_t *)param1;
	hal_video_adapter_t  *v_adp = (hal_video_adapter_t *)param2;
	commandLine_s *cml = &v_adp->cmd[enc2out->ch];
	video_ctx_t *ctx = (video_ctx_t *)arg;
	mm_context_t *mctx = (mm_context_t *)ctx->parent;
	mm_queue_item_t *output_item;

	u32 timestamp = xTaskGetTickCount();
	int is_output_ready = 0;

#if VIDEO_DEBUG
	if (enc2out->codec & CODEC_JPEG) {
		printf("jpeg in = 0x%X\r\n", enc2out->jpg_addr);
	} else if (enc2out->codec & CODEC_H264 || enc2out->codec & CODEC_HEVC) {
		printf("hevc/h264 in = 0x%X\r\n", enc2out->enc_addr);
	} else {
		printf("nv12/nv16/rgb in = 0x%X\r\n", enc2out->isp_addr);
	}
#endif

	if (enc2out->cmd_status != VOE_OK) {
		switch (enc2out->cmd_status) {
		case VOE_ENC_OVERFLOW:
			printf("VOE CH%d ENC overflow\n", enc2out->ch);
			break;
		default:
			printf("Error CH%d VOE status %x\n", enc2out->ch, enc2out->cmd_status);
			break;
		}
		return;
	}

	if (ctx->params.direct_output == 1) {
		goto show_log;
	}

	// Snapshot JPEG
	if (enc2out->codec & CODEC_JPEG && enc2out->jpg_len > 0) { // JPEG
		if (ctx->snapshot_cb != NULL) {
			dcache_invalidate_by_addr((uint32_t *)enc2out->jpg_addr, enc2out->jpg_len);
			dcache_invalidate_by_addr((uint32_t *)v_adp->outbuf[enc2out->ch], sizeof(hal_video_buf_s));
			ctx->snapshot_cb(enc2out->jpg_addr, enc2out->jpg_len);
			//if (cml->voe == 1) {
			//video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->jpg_len);
			//}
		} else {
			char *tempaddr;
			if (ctx->params.use_static_addr == 0) {
				tempaddr = (char *)malloc(enc2out->jpg_len);
				if (tempaddr == NULL) {
					if (cml->voe == 1) {
						video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->jpg_len);
					}
					printf("malloc fail = %d\r\n", enc2out->jpg_len);
					goto show_log;
				}
			}

			is_output_ready = xQueueReceive(mctx->output_recycle, (void *)&output_item, 10);
			if (is_output_ready) {
				dcache_invalidate_by_addr((uint32_t *)enc2out->jpg_addr, enc2out->jpg_len);
				dcache_invalidate_by_addr((uint32_t *)v_adp->outbuf[enc2out->ch], sizeof(hal_video_buf_s));
				if (ctx->params.use_static_addr) {
					output_item->data_addr = (char *)enc2out->jpg_addr;
				} else {
					output_item->data_addr = (char *)tempaddr;//malloc(enc2out->jpg_len);
					memcpy(output_item->data_addr, (char *)enc2out->jpg_addr, enc2out->jpg_len);
					if (cml->voe == 1) {
						video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->jpg_len);
					}
				}
				output_item->size = enc2out->jpg_len;
				output_item->timestamp = timestamp;
				output_item->hw_timestamp = enc2out->post_time;
				output_item->type = AV_CODEC_ID_MJPEG;
				output_item->index = 0;

				if (xQueueSend(mctx->output_ready, (void *)&output_item, 10) != pdTRUE) {
					video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->jpg_len);
				}

			} else {
				//printf("\r\n xQueueReceive fail \r\n");
				if (ctx->params.use_static_addr == 0) {
					free(tempaddr);
				}

				if (cml->voe == 1) {
					video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->jpg_len);
				}

			}
		}
	}

	if (enc2out->enc_len > 0 && (enc2out->codec & CODEC_H264 || enc2out->codec & CODEC_HEVC ||
								 enc2out->codec & CODEC_RGB || enc2out->codec & CODEC_NV12 ||
								 enc2out->codec & CODEC_NV16)) {
		char *tempaddr;
		if (ctx->params.use_static_addr == 0) {
			tempaddr = (char *)malloc(enc2out->enc_len);
			if (tempaddr == NULL) {
				printf("malloc fail = %d\r\n", enc2out->enc_len);
				if (cml->voe == 1) {
					if ((enc2out->codec & (CODEC_NV12 | CODEC_RGB | CODEC_NV16)) != 0) {
						video_ispbuf_release(enc2out->ch, enc2out->isp_addr);
					} else {
						video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->enc_len);
					}
				}
				goto show_log;
			}
		}

		is_output_ready = xQueueReceive(mctx->output_recycle, (void *)&output_item, 10);
		if (is_output_ready) {
			if (enc2out->codec == CODEC_H264) {
				output_item->type = AV_CODEC_ID_H264;
			} else if (enc2out->codec == CODEC_HEVC) {
				output_item->type = AV_CODEC_ID_H265;
			} else if (enc2out->codec == CODEC_RGB) {
				output_item->type = AV_CODEC_ID_RGB888;
			} else if (enc2out->codec == CODEC_NV12) {
				output_item->type = AV_CODEC_ID_UNKNOWN;
			} else if (enc2out->codec == CODEC_NV16) {
				output_item->type = AV_CODEC_ID_UNKNOWN;
			}

			if (enc2out->codec <= CODEC_JPEG) {
				dcache_invalidate_by_addr((uint32_t *)enc2out->enc_addr, enc2out->enc_len);
				dcache_invalidate_by_addr((uint32_t *)v_adp->outbuf[enc2out->ch], sizeof(hal_video_buf_s));

				if (enc2out->codec == CODEC_H264) {
					uint8_t *ptr = (uint8_t *)enc2out->enc_addr;
					if (ptr[0] != 0 || ptr[1] != 0) {
						printf("\r\nH264 stream error\r\n");
						printf("\r\n(%d/%d) %x %x %x %x\r\n", enc2out->enc_len, enc2out->finish, *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3));
					}
				}

				if (ctx->params.use_static_addr) {
					output_item->data_addr = (char *)enc2out->enc_addr;
				} else {
					output_item->data_addr = (char *)tempaddr;//malloc(enc2out->enc_len);
					memcpy(output_item->data_addr, (char *)enc2out->enc_addr, enc2out->enc_len);
					if (ctx->params.use_static_addr == 0) {
						video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->enc_len);
					}
				}

			} else {
				dcache_invalidate_by_addr((uint32_t *)enc2out->isp_addr, enc2out->enc_len);
				dcache_invalidate_by_addr((uint32_t *)v_adp->outbuf[enc2out->ch], sizeof(hal_video_buf_s));
				if (ctx->params.use_static_addr) {
					output_item->data_addr = (char *)enc2out->isp_addr;
				} else {
					output_item->data_addr = (char *)tempaddr;//malloc(enc2out->enc_len);
					memcpy(output_item->data_addr, (char *)enc2out->isp_addr, enc2out->enc_len);
					video_ispbuf_release(enc2out->ch, enc2out->isp_addr);
				}
			}

			output_item->size = enc2out->enc_len;
			output_item->timestamp = timestamp;
			output_item->hw_timestamp = enc2out->post_time;
			output_item->index = 0;

			if (xQueueSend(mctx->output_ready, (void *)&output_item, 10) != pdTRUE) {
				//printf("\r\n xQueueSend fail \r\n");
				if (enc2out->codec <= CODEC_JPEG) {
					video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->enc_len);
				} else {
					video_ispbuf_release(enc2out->ch, enc2out->isp_addr);
				}
			}

		} else {
			//printf("\r\n xQueueReceive fail \r\n");

			if (ctx->params.use_static_addr == 0) {
				free(tempaddr);
			}

			if (enc2out->codec <= CODEC_JPEG) {
				video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->enc_len);
			} else {
				video_ispbuf_release(enc2out->ch, enc2out->isp_addr);
			}
		}
	}

show_log:

	if (ctx->params.direct_output == 1) {

		if (enc2out->codec & (CODEC_H264 | CODEC_HEVC)) {
			printf("(%s)(%ldx%ld)(ch%d)(%s) Size(%6ld) QP(%2d) %3d ms %3d ms %3d ms CRC(0x%x)  \n"
				   , (enc2out->codec & CODEC_H264) != 0 ? "H264" : "HEVC"
				   , cml->width, cml->height, enc2out->ch
				   , (enc2out->type == VCENC_INTRA_FRAME) ? "I" : "P"
				   , enc2out->enc_len, enc2out->qp
				   , enc2out->pre_time >> 10
				   , enc2out->enc_time >> 10
				   , enc2out->post_time >> 10
				   , v_adp->crc32);

		}


		if (enc2out->codec & CODEC_JPEG && enc2out->jpg_len > 0) { // JPEG
			printf("(JPEG)(%ldx%ld)(ch%d)(I) Size(%6ld) QP(  ) %3d ms %3d ms %3d ms CRC(0x%x)  \n"
				   , cml->width, cml->height, enc2out->ch
				   , enc2out->jpg_len
				   , enc2out->pre_time >> 10
				   , enc2out->enc_time >> 10
				   , enc2out->post_time >> 10
				   , v_adp->crc32);
		}

		if (enc2out->codec & CODEC_RGB) {
			printf("(%s)(%ldx%ld)(ch%d)(%s) Size(%6ld) QP(%2d) %3d ms %3d ms %3d ms CRC(0x%x)  \n"
				   , "RGB"
				   , cml->width, cml->height, enc2out->ch
				   , (enc2out->type == VCENC_INTRA_FRAME) ? "I" : "P"
				   , enc2out->enc_len, enc2out->qp
				   , enc2out->pre_time >> 10
				   , enc2out->enc_time >> 10
				   , enc2out->post_time >> 10
				   , v_adp->crc32);

		}

		if (enc2out->codec & CODEC_NV12) {
			printf("(%s)(%ldx%ld)(ch%d)(%s) Size(%6ld) QP(%2d) %3d ms %3d ms %3d ms CRC(0x%x)  \n"
				   , "NV12"
				   , cml->width, cml->height, enc2out->ch
				   , (enc2out->type == VCENC_INTRA_FRAME) ? "I" : "P"
				   , enc2out->enc_len, enc2out->qp
				   , enc2out->pre_time >> 10
				   , enc2out->enc_time >> 10
				   , enc2out->post_time >> 10
				   , v_adp->crc32);

		}

		if (enc2out->codec & CODEC_NV16) {
			printf("(%s)(%ldx%ld)(ch%d)(%s) Size(%6ld) QP(%2d) %3d ms %3d ms %3d ms CRC(0x%x)  \n"
				   , "NV16"
				   , cml->width, cml->height, enc2out->ch
				   , (enc2out->type == VCENC_INTRA_FRAME) ? "I" : "P"
				   , enc2out->enc_len, enc2out->qp
				   , enc2out->pre_time >> 10
				   , enc2out->enc_time >> 10
				   , enc2out->post_time >> 10
				   , v_adp->crc32);

		}

	}

	if (ctx->params.direct_output == 1) {
		if ((enc2out->codec & (CODEC_H264 | CODEC_HEVC)) != 0) {
			video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->enc_len);
		} else if ((enc2out->codec & (CODEC_NV12 | CODEC_RGB | CODEC_NV16)) != 0) {
			video_ispbuf_release(enc2out->ch, enc2out->isp_addr);
		} else if ((enc2out->codec & CODEC_JPEG) != 0) {
			video_encbuf_release(enc2out->ch, enc2out->codec, enc2out->jpg_len);
		}
	}

	//close output task
	if (enc2out->finish == LAST_FRAME) {

	}
	incb = 0;
}


int video_control(void *p, int cmd, int arg)
{
	video_ctx_t *ctx = (video_ctx_t *)p;
	mm_context_t *mctx = (mm_context_t *)ctx->parent;
	mm_queue_item_t *tmp_item;

	switch (cmd) {
	case CMD_VIDEO_SET_PARAMS:
		memcpy(&ctx->params, (void *)arg, sizeof(video_params_t));
		break;
	case CMD_VIDEO_GET_PARAMS:
		memcpy((void *)arg, &ctx->params, sizeof(video_params_t));
		break;
	case CMD_VIDEO_SET_RCPARAM: {
		int ch = ctx->params.stream_id;
		video_ctrl(ctx->v_adp, ch, VIDEO_SET_RCPARAM, arg);
	}
	break;
	case CMD_VIDEO_STREAMID:
		ctx->params.stream_id = arg;
		break;
	case CMD_VIDEO_STREAM_START: {
		int ch = ctx->params.stream_id;
		video_start(ctx->v_adp, ch, 0);
	}
	break;
	case CMD_VIDEO_STREAM_STOP: {
		int ch = ctx->params.stream_id;
		//wait incb
		do {
			vTaskDelay(1);
		} while (incb);

		printf("leave cb\r\n");

		video_close(ctx->v_adp, ch);
	}
	break;
	case CMD_VIDEO_FORCE_IFRAME: {
		int ch = ctx->params.stream_id;
		video_ctrl(ctx->v_adp, ch, VIDEO_FORCE_IFRAME, arg);
	}
	break;
	case CMD_VIDEO_BPS: {
		int ch = ctx->params.stream_id;
		video_ctrl(ctx->v_adp, ch, VIDEO_BPS, arg);
	}
	break;
	case CMD_VIDEO_GOP: {
		int ch = ctx->params.stream_id;
		video_ctrl(ctx->v_adp, ch, VIDEO_GOP, arg);
	}
	break;
	case CMD_VIDEO_SNAPSHOT: {
		int ch = ctx->params.stream_id;
		video_ctrl(ctx->v_adp, ch, VIDEO_JPEG_OUTPUT, arg);
	}
	break;
	case CMD_VIDEO_YUV: {
		int ch = ctx->params.stream_id;
		int type = ctx->params.type;
		switch (type) {
		case 0:
			printf("wrong type %d\r\n", type);
			break;
		case 1:
			printf("wrong type %d\r\n", type);
			break;
		case 2:
			printf("wrong type %d\r\n", type);
			break;
		case 3:
			video_ctrl(ctx->v_adp, ch, VIDEO_NV12_OUTPUT, arg);
			break;
		case 4:
			video_ctrl(ctx->v_adp, ch, VIDEO_RGB_OUTPUT, arg);
			break;
		case 5:
			video_ctrl(ctx->v_adp, ch, VIDEO_NV16_OUTPUT, arg);
			break;
		case 6:
			printf("wrong type %d\r\n", type);
			break;
		case 7:
			printf("wrong type %d\r\n", type);
			break;
		}

	}
	break;
	case CMD_ISP_SET_RAWFMT: {
		int ch = ctx->params.stream_id;
		video_ctrl(ctx->v_adp, ch, VIDEO_ISP_SET_RAWFMT, arg);
	}
	break;
	case CMD_VIDEO_SNAPSHOT_CB:
		ctx->snapshot_cb = (int (*)(uint32_t, uint32_t))arg;
		break;
	case CMD_VIDEO_UPDATE:

		break;
	case CMD_VIDEO_SET_VOE_HEAP:
		if (video_alloc_heap(ctx->v_adp, arg) < 0) {
			printf("VOE buffer can't be allocated\r\n");
			while (1);
		}
		break;
	case CMD_VIDEO_PRINT_INFO: {
		int ch = ctx->params.stream_id;
		video_ctrl(ctx->v_adp, ch, VIDEO_PRINT_INFO, arg);
	}
	break;
	case CMD_VIDEO_APPLY: {
		int ch = arg;
		ctx->params.stream_id = ch;
		video_open(ctx->v_adp, &ctx->params, video_frame_complete_cb, ctx);
	}
	break;
	case CMD_VIDEO_MD_SET_ROI: {
		int *roi = (int *)arg;
		md_set(MD2_PARAM_ROI, roi);
	}
	break;
	case CMD_VIDEO_MD_SET_SENSITIVITY: {
		int sensitivity = arg;
		md_set(MD2_PARAM_SENSITIVITY, &sensitivity);
	}
	break;
	case CMD_VIDEO_MD_START: {
		if (hal_video_md_cb(md_output_cb) != OK) {
			printf("hal_video_md_cb_register fail\n");
		} else {
			md_start();
		}
	}
	break;
	case CMD_VIDEO_MD_STOP: {
		md_stop();
	}
	break;
	}
	return 0;
}

int video_handle(void *ctx, void *input, void *output)
{
	return 0;
}

void *video_destroy(void *p)
{
	video_ctx_t *ctx = (video_ctx_t *)p;
	int ch = ctx->params.stream_id;
	ctx->v_adp->ctx[ch] = NULL;

	video_deinit(ctx->v_adp);

	free(ctx);
	return NULL;
}


void *video_create(void *parent)
{
	video_ctx_t *ctx = malloc(sizeof(video_ctx_t));
	int iq_addr, sensor_addr;

	if (!ctx) {
		return NULL;
	}
	memset(ctx, 0, sizeof(video_ctx_t));

	ctx->parent = parent;

	if (USE_SENSOR == SENSOR_GC2053) {
		extern int _binary_iq_gc2053_bin_start[];
		iq_addr = _binary_iq_gc2053_bin_start;
		extern int _binary_sensor_gc2053_bin_start[];	// SENSOR binary address
		sensor_addr = _binary_sensor_gc2053_bin_start;
	} else if (USE_SENSOR == SENSOR_PS5258) {
		extern int _binary_iq_ps5258_bin_start[];
		iq_addr = _binary_iq_ps5258_bin_start;
		extern int _binary_sensor_ps5258_bin_start[];	// SENSOR binary address
		sensor_addr = _binary_sensor_ps5258_bin_start;
	} else if (USE_SENSOR == SENSOR_GC4653) {
		extern int _binary_iq_gc4653_bin_start[];
		iq_addr = _binary_iq_gc4653_bin_start;
		extern int _binary_sensor_gc4653_bin_start[];	// SENSOR binary address
		sensor_addr = _binary_sensor_gc4653_bin_start;
	} else if (USE_SENSOR == SENSOR_MIS2008) {
		extern int _binary_iq_mis2008_bin_start[];
		iq_addr = _binary_iq_mis2008_bin_start;
		extern int _binary_sensor_mis2008_bin_start[];	// SENSOR binary address
		sensor_addr = _binary_sensor_mis2008_bin_start;
	} else if (USE_SENSOR == SENSOR_IMX307) {
		extern int _binary_iq_imx307_bin_start[];
		iq_addr = _binary_iq_imx307_bin_start;
		extern int _binary_sensor_imx307_bin_start[];	// SENSOR binary address
		sensor_addr = _binary_sensor_imx307_bin_start;
	} else if (USE_SENSOR == SENSOR_IMX307HDR) {
		extern int _binary_iq_imx307hdr_bin_start[];
		iq_addr = _binary_iq_imx307hdr_bin_start;
		extern int _binary_sensor_imx307hdr_bin_start[];	// SENSOR binary address
		sensor_addr = _binary_sensor_imx307hdr_bin_start;
	} else {
		printf("unkown sensor\r\n");
	}

	ctx->v_adp = video_init(iq_addr, sensor_addr);
	printf("ctx->v_adp = 0x%X\r\n", ctx->v_adp);

	return ctx;
}

void *video_new_item(void *p)
{
	return NULL;
}

void *video_del_item(void *p, void *d)
{

	video_ctx_t *ctx = (video_ctx_t *)p;
	int ch = ctx->params.stream_id;

	if (ctx->params.use_static_addr == 0) {
		if (d) {
			free(d);
		}
	}

	return NULL;
}

void *video_voe_release_item(void *p, void *d, int length)
{
	video_ctx_t *ctx = (video_ctx_t *)p;
	mm_queue_item_t *free_item = (mm_queue_item_t *)d;
	int ch = ctx->params.stream_id;
	int codec = AV_CODEC_ID_UNKNOWN;
	switch (free_item->type) {
	case AV_CODEC_ID_H265:
		codec = CODEC_HEVC;
		break;
	case AV_CODEC_ID_H264:
		codec = CODEC_H264;
		break;
	case AV_CODEC_ID_MJPEG:
		codec = CODEC_JPEG;
		break;
	case AV_CODEC_ID_RGB888:
		codec = CODEC_RGB;
		break;
	}

	if (ctx->params.use_static_addr == 1) {
		if (free_item->type == AV_CODEC_ID_H264 || free_item->type == AV_CODEC_ID_H265 || free_item->type == AV_CODEC_ID_MJPEG) {
			video_encbuf_release(ch, codec, length);
		} else if (free_item->type == AV_CODEC_ID_RGB888) {
			video_ispbuf_release(ch, free_item->data_addr);
			rgb_lock = 0;
		} else {
			video_ispbuf_release(ch, free_item->data_addr);
		}
	}


	return NULL;
}

int video_voe_presetting(int v1_enable, int v1_w, int v1_h, int v1_bps, int v1_shapshot,
						 int v2_enable, int v2_w, int v2_h, int v2_bps, int v2_shapshot,
						 int v3_enable, int v3_w, int v3_h, int v3_bps, int v3_shapshot,
						 int v4_enable, int v4_w, int v4_h)
{
	int voe_heap_size = 0;
	isp_info_t info;

	if (USE_SENSOR == SENSOR_GC2053 || USE_SENSOR == SENSOR_PS5258 || USE_SENSOR == SENSOR_MIS2008 || USE_SENSOR == SENSOR_IMX307 ||
		USE_SENSOR == SENSOR_IMX307HDR) {
		info.sensor_width = 1920;
		info.sensor_height = 1080;
		info.sensor_fps = 15;
	} else if (USE_SENSOR == SENSOR_GC4653) {
		info.sensor_width = 2560;
		info.sensor_height = 1440;
		info.sensor_fps = 30;
	}

#if OSD_ENABLE
	info.osd_enable = 1;
#endif

#if MD_ENABLE
	info.md_enable = 1;
#endif

#if HDR_ENABLE
	info.hdr_enable = 1;
#endif

	video_set_isp_info(&info);

	voe_heap_size =  video_buf_calc(v1_enable, v1_w, v1_h, v1_bps, v1_shapshot,
									v2_enable, v2_w, v2_h, v2_bps, v2_shapshot,
									v3_enable, v3_w, v3_h, v3_bps, v3_shapshot,
									v4_enable, v4_w, v4_h);

	return voe_heap_size;
}

mm_module_t video_module = {
	.create = video_create,
	.destroy = video_destroy,
	.control = video_control,
	.handle = video_handle,

	.new_item = video_new_item,
	.del_item = video_del_item,
	.rsz_item = NULL,
	.vrelease_item = video_voe_release_item,

	.output_type = MM_TYPE_VDSP,    // output for video algorithm
	.module_type = MM_TYPE_VSRC,    // module type is video source
	.name = "VIDEO"
};
