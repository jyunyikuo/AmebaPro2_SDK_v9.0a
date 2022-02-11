/******************************************************************************
*
* Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"

#include "module_array.h"
#include "module_mbnssd.h"

#include "avcodec.h"

#include "input_image_640x360x3.h"
// TODO: move model id to proper header


#define NN_WIDTH	640
#define NN_HEIGHT	360


static mm_context_t *array_ctx            = NULL;
static mm_context_t *ssd_ctx            = NULL;
static mm_siso_t *siso_array_ssd         = NULL;

static array_params_t h264_array_params = {
	.type = AVMEDIA_TYPE_VIDEO,
	.codec_id = AV_CODEC_ID_RGB888,
	.mode = ARRAY_MODE_LOOP,
	.u = {
		.v = {
			.fps    = 7,
		}
	}
};

#define ROI_WIDTH		NN_WIDTH
#define ROI_HEIGHT		NN_HEIGHT

static mbnssd_param_t mbnssd_params = {
	.roi = {
		.xmin = (NN_WIDTH - ROI_WIDTH) / 2,
		.ymin = (NN_HEIGHT - ROI_HEIGHT) / 2,
		.xmax = (NN_WIDTH + ROI_WIDTH) / 2,
		.ymax = (NN_HEIGHT + ROI_HEIGHT) / 2,
	},
	.in_width = NN_WIDTH,
	.in_height = NN_HEIGHT,
};

void mmf2_video_example_array_mbnssd_init(void)
{
	// Video array input (H264)
	array_t array;
	array.data_addr = (uint32_t) testRGB_640x360;
	array.data_len = (uint32_t) 640 * 480 * 3;
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
		goto mmf2_example_array_ssd_fail;
	}

	// SSD
	ssd_ctx = mm_module_open(&mbnssd_module);
	if (ssd_ctx) {
		mm_module_ctrl(ssd_ctx, CMD_SSD_SET_PARAMS, (int)&mbnssd_params);
	} else {
		rt_printf("SSD open fail\n\r");
		goto mmf2_example_array_ssd_fail;
	}
	rt_printf("SSD opened\n\r");

	//--------------Link---------------------------
	siso_array_ssd = siso_create();
	if (siso_array_ssd) {
		siso_ctrl(siso_array_ssd, MMIC_CMD_ADD_INPUT, (uint32_t)array_ctx, 0);
		siso_ctrl(siso_array_ssd, MMIC_CMD_ADD_OUTPUT, (uint32_t)ssd_ctx, 0);
		siso_start(siso_array_ssd);
	} else {
		rt_printf("siso_array_ssd open fail\n\r");
		goto mmf2_example_array_ssd_fail;
	}
	rt_printf("siso_array_ssd started\n\r");

	return;
mmf2_example_array_ssd_fail:

	return;
}



