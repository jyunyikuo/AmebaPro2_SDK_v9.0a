/******************************************************************************
*
* Copyright(c) 2007 - 2021 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "module_video.h"
#include "mmf2_pro2_video_config.h"
#include "video_example_media_framework.h"

#include "module_vipnn.h"

#include "avcodec.h"

#include "input_image_640x360x3.h"

#include "model_mbnetssd.h"
#include "model_yolov3t.h"

#include "module_rtsp2.h"

#include "hal_video.h"
#include "hal_isp.h"



/*****************************************************************************
* ISP channel : 4
* Video type  : RGB
*****************************************************************************/
#define RTSP_CHANNEL 0
#define RTSP_RESOLUTION VIDEO_FHD
#define RTSP_FPS 15
#define RTSP_GOP 15
#define RTSP_BPS 1*1024*1024
#define VIDEO_RCMODE 2 // 1: CBR, 2: VBR

#define USE_H265 0

#if USE_H265
#include "sample_h265.h"
#define RTSP_TYPE VIDEO_HEVC
#define RTSP_CODEC AV_CODEC_ID_H265
#else
#include "sample_h264.h"
#define RTSP_TYPE VIDEO_H264
#define RTSP_CODEC AV_CODEC_ID_H264
#endif

#if RTSP_RESOLUTION == VIDEO_VGA
#define RTSP_WIDTH	640
#define RTSP_HEIGHT	480
#elif RTSP_RESOLUTION == VIDEO_HD
#define RTSP_WIDTH	1280
#define RTSP_HEIGHT	720
#elif RTSP_RESOLUTION == VIDEO_FHD
#define RTSP_WIDTH	1920
#define RTSP_HEIGHT	1080
#endif

static video_params_t video_v1_params = {
	.stream_id 		= RTSP_CHANNEL,
	.type 			= RTSP_TYPE,
	.resolution 	= RTSP_RESOLUTION,
	.width 			= RTSP_WIDTH,
	.height 		= RTSP_HEIGHT,
	.bps            = RTSP_BPS,
	.fps 			= RTSP_FPS,
	.gop 			= RTSP_GOP,
	.rc_mode        = VIDEO_RCMODE,
	.use_static_addr = 1
};


static rtsp2_params_t rtsp2_v1_params = {
	.type = AVMEDIA_TYPE_VIDEO,
	.u = {
		.v = {
			.codec_id = RTSP_CODEC,
			.fps      = RTSP_FPS,
			.bps      = RTSP_BPS
		}
	}
};


#define NN_CHANNEL 4
#define NN_RESOLUTION VIDEO_VGA //VIDEO_WVGA
#define NN_FPS 10
#define NN_GOP 10
#define NN_BPS 1024*1024
#define NN_EXCUTE_FPS 5

#define NN_TYPE VIDEO_RGB

#if NN_RESOLUTION == VIDEO_VGA
#define NN_WIDTH	640
#define NN_HEIGHT	480
#elif NN_RESOLUTION == VIDEO_WVGA
#define NN_WIDTH	640
#define NN_HEIGHT	360
#endif

static video_params_t video_v4_params = {
	.stream_id 		= NN_CHANNEL,
	.type 			= NN_TYPE,
	.resolution	 	= NN_RESOLUTION,
	.width 			= NN_WIDTH,
	.height 		= NN_HEIGHT,
	.bps 			= NN_BPS,
	.fps 			= NN_FPS,
	.gop 			= NN_GOP,
	.direct_output 	= 0,
	.use_static_addr = 1
};



#include "module_array.h"
static array_params_t h264_array_params = {
	.type = AVMEDIA_TYPE_VIDEO,
	.codec_id = AV_CODEC_ID_RGB888,
	.mode = ARRAY_MODE_LOOP,
	.u = {
		.v = {
			.fps    = 5,
		}
	}
};

static nn_data_param_t roi_wvga = {
	.img = {
		.width = NN_WIDTH,
		.height = NN_HEIGHT,
		.rgb = 1,
		.roi = {
			.xmin = 0,
			.ymin = 0,
			.xmax = NN_WIDTH,
			.ymax = NN_HEIGHT,
		}
	}
};

static float nn_confidence_thresh = 0.4;
static float nn_nms_thresh = 0.3;

#define V1_ENA 1
#define V4_ENA 1
#define V4_SIM 0

static mm_context_t *array_ctx            = NULL;
static mm_context_t *video_v1_ctx			= NULL;
static mm_context_t *rtsp2_v1_ctx			= NULL;
static mm_siso_t *siso_video_rtsp_v1			= NULL;

static mm_context_t *video_rgb_ctx			= NULL;
static mm_context_t *vipnn_ctx            = NULL;
static mm_siso_t *siso_array_vipnn         = NULL;


//--------------------------------------------
// Draw Rect
//--------------------------------------------
static obj_ctrl_s sw_object;
static void nn_set_object(void *p, void *img_param)
{
	int i = 0;
	objdetect_res_t *res = (objdetect_res_t *)p;
	nn_data_param_t *im = (nn_data_param_t *)img_param;

	if (!p || !img_param)	{
		return;
	}

	int im_h = RTSP_HEIGHT;
	int im_w = RTSP_WIDTH;
	float ratio_h = (float)im_h / (float)im->img.height;
	float ratio_w = (float)im_w / (float)im->img.width;
	int roi_h = (int)((im->img.roi.ymax - im->img.roi.ymin) * ratio_h);
	int roi_w = (int)((im->img.roi.xmax - im->img.roi.xmin) * ratio_w);
	int roi_x = (int)(im->img.roi.xmin * ratio_w);	// 0
	int roi_y = (int)(im->img.roi.ymin * ratio_h);	// 0

	printf("object num = %d\r\n", res->obj_num);
	if (res->obj_num > 0) {
		sw_object.objDetectNumber = 0;
		for (i = 0; i < res->obj_num; i++) {
			if (sw_object.objDetectNumber == 10) {
				break;
			}

			sw_object.objDetectNumber++;

			sw_object.objTopY[i] = (int)(res->result[6 * i + 3] * roi_h) + roi_y;
			if (sw_object.objTopY[i] < 0) {
				sw_object.objTopY[i] = 0;
			} else if (sw_object.objTopY[i] >= im_h) {
				sw_object.objTopY[i] = im_h - 1;
			}

			sw_object.objTopX[i] = (int)(res->result[6 * i + 2] * roi_w) + roi_x;
			if (sw_object.objTopX[i] < 0) {
				sw_object.objTopX[i] = 0;
			} else if (sw_object.objTopX[i] >= im_w) {
				sw_object.objTopX[i] = im_w - 1;
			}

			sw_object.objBottomY[i] = (int)(res->result[6 * i + 5] * roi_h) + roi_y;
			if (sw_object.objBottomY[i] < 0) {
				sw_object.objBottomY[i] = 0;
			} else if (sw_object.objBottomY[i] >= im_h) {
				sw_object.objBottomY[i] = im_h - 1;
			}

			sw_object.objBottomX[i] = (int)(res->result[6 * i + 4] * roi_w) + roi_x;
			if (sw_object.objBottomX[i] < 0) {
				sw_object.objBottomX[i] = 0;
			} else if (sw_object.objBottomX[i] >= im_w) {
				sw_object.objBottomX[i] = im_w - 1;
			}

			printf("%d,c%d:%d %d %d %d\n\r", i, (int)res->result[6 * i ], sw_object.objTopX[i], sw_object.objTopY[i], sw_object.objBottomX[i], sw_object.objBottomY[i]);
		}
	} else {
		sw_object.objDetectNumber = 0;
	}

	if (sw_object.objDetectNumber == 0) {
		sw_object.objDetectNumber = 1;
		sw_object.objTopY[0] = 0;
		sw_object.objTopX[0] = 0;
		sw_object.objBottomY[0] = 0;
		sw_object.objBottomX[0] = 0;
	}
	hal_video_obj_region(&sw_object, RTSP_CHANNEL);
}

void mmf2_video_example_vipnn_rtsp_init(void)
{

	int voe_heap_size = video_voe_presetting(V1_ENA, RTSP_WIDTH, RTSP_HEIGHT, RTSP_BPS, 0,
						0, 0, 0, 0, 0,
						0, 0, 0, 0, 0,
						V4_ENA, NN_WIDTH, NN_HEIGHT);

	printf("\r\n voe heap size = %d\r\n", voe_heap_size);
#if V1_ENA
	video_v1_ctx = mm_module_open(&video_module);
	if (video_v1_ctx) {
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_SET_VOE_HEAP, voe_heap_size);
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_SET_PARAMS, (int)&video_v1_params);
		mm_module_ctrl(video_v1_ctx, MM_CMD_SET_QUEUE_LEN, RTSP_FPS);
		mm_module_ctrl(video_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_APPLY, RTSP_CHANNEL);	// start channel 0
	} else {
		rt_printf("video open fail\n\r");
		goto mmf2_example_vnn_rtsp_fail;
	}


	// encode_rc_parm_t rc_parm;
	// rc_parm.minQp = 28;
	// rc_parm.maxQp = 45;

	// mm_module_ctrl(video_v1_ctx, CMD_VIDEO_SET_RCPARAM, (int)&rc_parm);

	rtsp2_v1_ctx = mm_module_open(&rtsp2_module);
	if (rtsp2_v1_ctx) {
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v1_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_STREAMMING, ON);
	} else {
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_example_vnn_rtsp_fail;
	}
#endif
#if V4_ENA
#if V4_SIM==0
	video_rgb_ctx = mm_module_open(&video_module);
	if (video_rgb_ctx) {
		//mm_module_ctrl(video_rgb_ctx, CMD_VIDEO_SET_VOE_HEAP, voe_heap_size);
		mm_module_ctrl(video_rgb_ctx, CMD_VIDEO_SET_PARAMS, (int)&video_v4_params);
		mm_module_ctrl(video_rgb_ctx, MM_CMD_SET_QUEUE_LEN, 2);
		mm_module_ctrl(video_rgb_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(video_rgb_ctx, CMD_VIDEO_APPLY, NN_CHANNEL);	// start channel 4
	} else {
		rt_printf("video open fail\n\r");
		goto mmf2_example_vnn_rtsp_fail;
	}
	mm_module_ctrl(video_rgb_ctx, CMD_VIDEO_YUV, 2);
	// VOE 0.7.0 work around, will removed
	vTaskDelay(1000);
#else
	array_t array;
	array.data_addr = (uint32_t) testRGB_640x360;
	array.data_len = (uint32_t) 640 * 360 * 3;
	array_ctx = mm_module_open(&array_module);
	if (array_ctx) {
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_PARAMS, (int)&h264_array_params);
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_ARRAY, (int)&array);
		mm_module_ctrl(array_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(array_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(array_ctx, CMD_ARRAY_APPLY, 0);
		mm_module_ctrl(array_ctx, CMD_ARRAY_STREAMING, 1);	// streamming on
	} else {
		rt_printf("ARRAY open fail\n\r");
		goto mmf2_example_vnn_rtsp_fail;
	}
#endif
#if 1
	// VIPNN
	vipnn_ctx = mm_module_open(&vipnn_module);
	if (vipnn_ctx) {
		//mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&mbnetssd);
		//mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_IN_PARAMS, (int)&roi_wvga);
		//mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&yolov3_tiny);
		//mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&mbnetssd_fwfs);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&yolov3_tiny_fwfs);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_IN_PARAMS, (int)&roi_wvga);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_DISPPOST, (int)nn_set_object);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_CONFIDENCE_THRES, (int)&nn_confidence_thresh);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_NMS_THRES, (int)&nn_nms_thresh);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_APPLY, 0);
	} else {
		rt_printf("VIPNN open fail\n\r");
		goto mmf2_example_vnn_rtsp_fail;
	}
	rt_printf("VIPNN opened\n\r");
#endif
#endif

	//--------------Link---------------------------
#if V1_ENA
	siso_video_rtsp_v1 = siso_create();
	if (siso_video_rtsp_v1) {
		siso_ctrl(siso_video_rtsp_v1, MMIC_CMD_ADD_INPUT, (uint32_t)video_v1_ctx, 0);
		siso_ctrl(siso_video_rtsp_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_v1_ctx, 0);
		siso_start(siso_video_rtsp_v1);
	} else {
		rt_printf("siso2 open fail\n\r");
		goto mmf2_example_vnn_rtsp_fail;
	}
#endif
#if 1 //V4_ENA
	siso_array_vipnn = siso_create();
	if (siso_array_vipnn) {
#if V4_SIM==0
		siso_ctrl(siso_array_vipnn, MMIC_CMD_ADD_INPUT, (uint32_t)video_rgb_ctx, 0);
		siso_ctrl(siso_array_vipnn, MMIC_CMD_SET_STACKSIZE, (uint32_t)1024 * 64, 0);
		siso_ctrl(siso_array_vipnn, MMIC_CMD_SET_TASKPRIORITY, 3, 0);
#else
		siso_ctrl(siso_array_vipnn, MMIC_CMD_ADD_INPUT, (uint32_t)array_ctx, 0);
#endif
		siso_ctrl(siso_array_vipnn, MMIC_CMD_ADD_OUTPUT, (uint32_t)vipnn_ctx, 0);
		siso_start(siso_array_vipnn);
	} else {
		rt_printf("siso_array_vipnn open fail\n\r");
		goto mmf2_example_vnn_rtsp_fail;
	}
	rt_printf("siso_array_vipnn started\n\r");
#endif

	return;
mmf2_example_vnn_rtsp_fail:

	return;
}