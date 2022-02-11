/******************************************************************************
*
* Copyright(c) 2007 - 2021 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "platform_opts.h"

#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_miso.h"

#include "avcodec.h"
#include "module_video.h"
#include "module_audio.h"
#include "module_g711.h"
#include "mmf2_pro2_video_config.h"
#include "video_example_media_framework.h"

#include "example_kvs_webrtc_mmf.h"
#include "module_kvs_webrtc.h"
#include "sample_config_webrtc.h"

/*****************************************************************************
* ISP channel : 0
* Video type  : H264/HEVC
*****************************************************************************/

#define V1_CHANNEL 0
#define V1_RESOLUTION VIDEO_HD
#define V1_FPS 30
#define V1_GOP 30
#define V1_BPS 512*1024
#define V1_RCMODE 1 // 1: CBR, 2: VBR

#define USE_H265 0

#if USE_H265
#define VIDEO_TYPE VIDEO_HEVC
#define VIDEO_CODEC AV_CODEC_ID_H265
#else
#define VIDEO_TYPE VIDEO_H264
#define VIDEO_CODEC AV_CODEC_ID_H264
#endif

#if V1_RESOLUTION == VIDEO_VGA
#define V1_WIDTH	640
#define V1_HEIGHT	480
#elif V1_RESOLUTION == VIDEO_HD
#define V1_WIDTH	1280
#define V1_HEIGHT	720
#elif V1_RESOLUTION == VIDEO_FHD
#define V1_WIDTH	1920
#define V1_HEIGHT	1080
#endif

static mm_context_t *video_v1_ctx           = NULL;
static mm_context_t *audio_ctx              = NULL;
static mm_context_t *g711e_ctx              = NULL;
static mm_context_t *g711d_ctx              = NULL;

static mm_context_t *kvs_webrtc_v1_a1_ctx   = NULL;

static mm_siso_t *siso_audio_g711e          = NULL;
static mm_siso_t *siso_webrtc_g711d         = NULL;
static mm_siso_t *siso_g711d_audio          = NULL;
static mm_miso_t *miso_h264_g711_kvs_v1_a1  = NULL;

static video_params_t video_v1_params = {
	.stream_id = V1_CHANNEL,
	.type = VIDEO_TYPE,
	.resolution = V1_RESOLUTION,
	.width = V1_WIDTH,
	.height = V1_HEIGHT,
	.bps = V1_BPS,
	.fps = V1_FPS,
	.gop = V1_GOP,
	.rc_mode = V1_RCMODE,
	.use_static_addr = 1
};

static audio_params_t audio_params = {
	.sample_rate = ASR_8KHZ,
	.word_length = WL_16BIT,
	.mic_gain    = MIC_40DB,
	.channel     = 1,
	.enable_aec  = 0
};

static g711_params_t g711e_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_ENCODE
};

static g711_params_t g711d_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_DECODE
};

#include "wifi_conf.h"
#include "lwip_netconf.h"
#define wifi_wait_time 500 //Here we wait 5 second to wiat the fast connect 
static void common_init()
{
	uint32_t wifi_wait_count = 0;

	while (!((wifi_get_join_status() == RTW_JOINSTATUS_SUCCESS) && (*(u32 *)LwIP_GetIP(0) != IP_ADDR_INVALID))) {
		vTaskDelay(10);
		wifi_wait_count++;
		if (wifi_wait_count == wifi_wait_time) {
			printf("\r\nuse ATW0, ATW1, ATWC to make wifi connection\r\n");
			printf("wait for wifi connection...\r\n");
		}
	}
}

void example_kvs_webrtc_mmf_thread(void *param)
{
	common_init();

	int voe_heap_size = video_voe_presetting(1, V1_WIDTH, V1_HEIGHT, V1_BPS, 0,
						0, 0, 0, 0, 0,
						0, 0, 0, 0, 0,
						0, 0, 0);

	printf("\r\n voe heap size = %d\r\n", voe_heap_size);

	video_v1_ctx = mm_module_open(&video_module);
	if (video_v1_ctx) {
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_SET_VOE_HEAP, voe_heap_size);
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_SET_PARAMS, (int)&video_v1_params);
		mm_module_ctrl(video_v1_ctx, MM_CMD_SET_QUEUE_LEN, 10);
		mm_module_ctrl(video_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_APPLY, V1_CHANNEL);	// start channel 0
	} else {
		rt_printf("video open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}

	audio_ctx = mm_module_open(&audio_module);
	if (audio_ctx) {
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_DAC_GAIN, (int)0x40);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	} else {
		rt_printf("AUDIO open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}

	g711e_ctx = mm_module_open(&g711_module);
	if (g711e_ctx) {
		mm_module_ctrl(g711e_ctx, CMD_G711_SET_PARAMS, (int)&g711e_params);
		mm_module_ctrl(g711e_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711e_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711e_ctx, CMD_G711_APPLY, 0);
	} else {
		rt_printf("G711 open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}

	kvs_webrtc_v1_a1_ctx = mm_module_open(&kvs_webrtc_module);
	if (kvs_webrtc_v1_a1_ctx) {
		mm_module_ctrl(kvs_webrtc_v1_a1_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(kvs_webrtc_v1_a1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(kvs_webrtc_v1_a1_ctx, CMD_KVS_WEBRTC_SET_APPLY, 0);
	} else {
		rt_printf("KVS open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}

	siso_audio_g711e = siso_create();
	if (siso_audio_g711e) {
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711e_ctx, 0);
		siso_start(siso_audio_g711e);
	} else {
		rt_printf("siso_audio_g711e open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}

	miso_h264_g711_kvs_v1_a1 = miso_create();
	if (miso_h264_g711_kvs_v1_a1) {
		miso_ctrl(miso_h264_g711_kvs_v1_a1, MMIC_CMD_ADD_INPUT0, (uint32_t)video_v1_ctx, 0);
		miso_ctrl(miso_h264_g711_kvs_v1_a1, MMIC_CMD_ADD_INPUT1, (uint32_t)g711e_ctx, 0);
		miso_ctrl(miso_h264_g711_kvs_v1_a1, MMIC_CMD_ADD_OUTPUT, (uint32_t)kvs_webrtc_v1_a1_ctx, 0);
		miso_start(miso_h264_g711_kvs_v1_a1);
	} else {
		rt_printf("miso_h264_g711_kvs_v1_a1 open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}
	rt_printf("miso started\n\r");

#ifdef ENABLE_AUDIO_SENDRECV
	g711d_ctx = mm_module_open(&g711_module);
	if (g711d_ctx) {
		mm_module_ctrl(g711d_ctx, CMD_G711_SET_PARAMS, (int)&g711d_params);
		mm_module_ctrl(g711d_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711d_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711d_ctx, CMD_G711_APPLY, 0);
	} else {
		rt_printf("G711 open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}

	siso_webrtc_g711d = siso_create();
	if (siso_webrtc_g711d) {
		siso_ctrl(siso_webrtc_g711d, MMIC_CMD_ADD_INPUT, (uint32_t)kvs_webrtc_v1_a1_ctx, 0);
		siso_ctrl(siso_webrtc_g711d, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711d_ctx, 0);
		siso_start(siso_webrtc_g711d);
	} else {
		rt_printf("siso2 open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}

	siso_g711d_audio = siso_create();
	if (siso_g711d_audio) {
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_INPUT, (uint32_t)g711d_ctx, 0);
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_g711d_audio);
	} else {
		rt_printf("siso3 open fail\n\r");
		goto example_kvs_webrtc_mmf;
	}
#endif

example_kvs_webrtc_mmf:

	// TODO: exit condition or signal
	while (1) {
		vTaskDelay(1000);
	}
}

void example_kvs_webrtc_mmf(void)
{
	/*user can start their own task here*/
	if (xTaskCreate(example_kvs_webrtc_mmf_thread, ((const char *)"example_kvs_webrtc_mmf_thread"), 4096, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_kvs_webrtc_mmf_thread: Create Task Error\n");
	}
}