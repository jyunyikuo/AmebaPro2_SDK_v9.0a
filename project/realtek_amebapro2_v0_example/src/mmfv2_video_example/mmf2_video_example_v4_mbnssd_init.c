/******************************************************************************
*
* Copyright(c) 2007 - 2021 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_pro2_video_config.h"
#include "video_example_media_framework.h"

#include "input_image_640x360x3.h"

#include "module_video.h"
#include "module_mbnssd.h"
#include "module_rtsp2.h"
#include "module_array.h"

#include "hal_video.h"
#include "hal_isp.h"


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

/*****************************************************************************
* ISP channel : 4
* Video type  : RGB
*****************************************************************************/

#define NN_CHANNEL 4
#define NN_RESOLUTION VIDEO_WVGA //VIDEO_WVGA
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

#define ROI_WIDTH		NN_WIDTH
#define ROI_HEIGHT		NN_HEIGHT

mbnssd_param_t mbnssd_params = {
	.roi = {
		.xmin = (NN_WIDTH - ROI_WIDTH) / 2,
		.ymin = (NN_HEIGHT - ROI_HEIGHT) / 2,
		.xmax = (NN_WIDTH + ROI_WIDTH) / 2,
		.ymax = (NN_HEIGHT + ROI_HEIGHT) / 2,
	},
	.in_width = NN_WIDTH,
	.in_height = NN_HEIGHT,
};


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

static mm_context_t *array_ctx          = NULL;
static mm_context_t *video_v1_ctx			= NULL;
static mm_context_t *rtsp2_v1_ctx			= NULL;
static mm_siso_t *siso_video_rtsp_v1		= NULL;

static mm_context_t *video_rgb_ctx			= NULL;
static mm_context_t *ssd_ctx            	= NULL;
static mm_siso_t *siso_v4_ssd         		= NULL;

static obj_ctrl_s sw_object;
static void mmf2_osd_draw(int xmin, int ymin, int xmax, int ymax, char *str, int op_mode)
{
	int im_h = RTSP_HEIGHT;
	int im_w = RTSP_WIDTH;
	int in_h = ROI_HEIGHT;
	int in_w = ROI_WIDTH;

	float ratio_h = (float)im_h / (float)in_h;
	float ratio_w = (float)im_w / (float)in_w;

#define SATVAL(val, min, max)	val=val>(min)?(val<(max)?val:(max)):(min)

	// op mode , 0: normal draw (counter increase), 1: reset counter 2: draw 3:clean
	if (op_mode == 0) {
		int idx = sw_object.objDetectNumber;
		if (idx == 10) {
			return;
		}

		printf("%s, %d %d %d %d\n\r", str, xmin, ymin, xmax, ymax);

		xmin = (int)((float)xmin * ratio_w);
		xmax = (int)((float)xmax * ratio_w);
		ymin = (int)((float)ymin * ratio_h);
		ymax = (int)((float)ymax * ratio_h);

		SATVAL(xmin, 0, im_w - 1);
		SATVAL(xmax, 0, im_w - 1);
		SATVAL(ymin, 0, im_h - 1);
		SATVAL(ymax, 0, im_h - 1);

		sw_object.objTopY[idx] = ymin;
		sw_object.objTopX[idx] = xmin;
		sw_object.objBottomY[idx] = ymax;
		sw_object.objBottomX[idx] = xmax;

		sw_object.objDetectNumber++;
	} else if (op_mode == 1) {
		sw_object.objDetectNumber = 0;
	} else if (op_mode == 2) {
		if (sw_object.objDetectNumber == 0) {
			sw_object.objDetectNumber = 1;
			sw_object.objTopY[0] = 0;
			sw_object.objTopX[0] = 0;
			sw_object.objBottomY[0] = 0;
			sw_object.objBottomX[0] = 0;
		}
		hal_video_obj_region(&sw_object, RTSP_CHANNEL);
	} else {
		//sw_object.objDetectNumber = 0;
		sw_object.objDetectNumber = 1;
		sw_object.objTopY[0] = 0;
		sw_object.objTopX[0] = 0;
		sw_object.objBottomY[0] = 0;
		sw_object.objBottomX[0] = 0;
		hal_video_obj_region(&sw_object, RTSP_CHANNEL);
	}

}

#define V1_ENA 1
#define V4_ENA 1
#define V4_SIM 0

#if V4_SIM==0
static TaskHandle_t rgbshot_thread = NULL;

static void rgbshot_control_thread(void *param)
{
	int shanpshot_time = 1000 / NN_EXCUTE_FPS;
	vTaskDelay(shanpshot_time);
	while (1) {
		mm_module_ctrl(video_rgb_ctx, CMD_VIDEO_YUV, 1);
		vTaskDelay(shanpshot_time);
	}
}
#endif

void mmf2_video_example_v4_mbnssd_init(void)
{

	int voe_heap_size = video_voe_presetting(
							1, RTSP_WIDTH, RTSP_HEIGHT, RTSP_BPS, 0,
							0, 0, 0, 0, 0,
							0, 0, 0, 0, 0,
							1, NN_WIDTH, NN_HEIGHT);

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
		goto mmf2_example_v4_ssd_fail;
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
		goto mmf2_example_v4_ssd_fail;
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
		goto mmf2_example_v4_ssd_fail;
	}
	mm_module_ctrl(video_rgb_ctx, CMD_VIDEO_YUV, 2);
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
		goto mmf2_example_v4_ssd_fail;
	}
#endif


	// SSD
	ssd_ctx = mm_module_open(&mbnssd_module);
	if (ssd_ctx) {
		mm_module_ctrl(ssd_ctx, CMD_SSD_SET_PARAMS, (int)&mbnssd_params);
		mm_module_ctrl(ssd_ctx, CMD_SSD_SET_OSD_DRAW, (int)mmf2_osd_draw);
	} else {
		rt_printf("SSD open fail\n\r");
		goto mmf2_example_v4_ssd_fail;
	}
	rt_printf("SSD opened\n\r");
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
		goto mmf2_example_v4_ssd_fail;
	}
#endif
#if V4_ENA
	siso_v4_ssd = siso_create();
	if (siso_v4_ssd) {
#if V4_SIM==0
		siso_ctrl(siso_v4_ssd, MMIC_CMD_ADD_INPUT, (uint32_t)video_rgb_ctx, 0);
		siso_ctrl(siso_v4_ssd, MMIC_CMD_SET_STACKSIZE, (uint32_t)1024 * 64, 0);
		siso_ctrl(siso_v4_ssd, MMIC_CMD_SET_TASKPRIORITY, 3, 0);
#else
		siso_ctrl(siso_v4_ssd, MMIC_CMD_ADD_INPUT, (uint32_t)array_ctx, 0);
#endif
		siso_ctrl(siso_v4_ssd, MMIC_CMD_ADD_OUTPUT, (uint32_t)ssd_ctx, 0);
		siso_start(siso_v4_ssd);
	} else {
		rt_printf("siso_v4_ssd open fail\n\r");
		goto mmf2_example_v4_ssd_fail;
	}
	rt_printf("siso_v4_ssd started\n\r");
#endif


	//if (xTaskCreate(rgbshot_control_thread, ((const char *)"rgbshot_store"), 4096, NULL, tskIDLE_PRIORITY + 1, &rgbshot_thread) != pdPASS) {
	//	printf("\n\r%s xTaskCreate failed", __FUNCTION__);
	//}


	return;
mmf2_example_v4_ssd_fail:

	return;
}
