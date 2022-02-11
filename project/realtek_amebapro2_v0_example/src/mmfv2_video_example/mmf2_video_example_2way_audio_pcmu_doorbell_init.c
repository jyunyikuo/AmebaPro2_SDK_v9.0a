/******************************************************************************
*
* Copyright(c) 2007 - 2021 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_miso.h"

#include "module_video.h"
#include "module_rtsp2.h"
#include "module_audio.h"
#include "module_g711.h"
#include "module_rtp.h"
#include "module_array.h"

#include "sample_doorbell_pcmu.h"
#include "mmf2_pro2_video_config.h"
#include "video_example_media_framework.h"

/*****************************************************************************
* ISP channel : 0,1
* Video type  : H264/HEVC
*****************************************************************************/

#define V1_CHANNEL 0
#if USE_SENSOR == SENSOR_GC4653
#define V1_RESOLUTION VIDEO_2K
#define V1_FPS 15
#define V1_GOP 15
#else
#define V1_RESOLUTION VIDEO_FHD
#define V1_FPS 30
#define V1_GOP 30
#endif
#define V1_BPS 2*1024*1024
#define V1_RCMODE 2 // 1: CBR, 2: VBR

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
#elif V1_RESOLUTION == VIDEO_2K
#define V1_WIDTH	2560
#define V1_HEIGHT	1440
#endif

static mm_context_t *video_v1_ctx			= NULL;
static mm_context_t *rtsp2_v1_ctx			= NULL;
static mm_context_t *audio_ctx				= NULL;
static mm_context_t *g711e_ctx				= NULL;
static mm_context_t *g711d_ctx				= NULL;
static mm_context_t *rtp_ctx				= NULL;
static mm_context_t *array_ctx				= NULL;

static mm_siso_t *siso_audio_g711e			= NULL;
static mm_miso_t *miso_video_g711_rtsp		= NULL;
static mm_miso_t *miso_rtp_array_g711d		= NULL;
static mm_siso_t *siso_g711d_audio			= NULL;

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
	.dmic_l_gain    = DMIC_BOOST_24DB,
	.dmic_r_gain    = DMIC_BOOST_24DB,
	.use_mic_type   = USE_AUDIO_AMIC,
	.channel     = 1,
	.enable_aec  = 0
};

static g711_params_t g711e_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_ENCODE
};

static rtsp2_params_t rtsp2_v1_params = {
	.type = AVMEDIA_TYPE_VIDEO,
	.u = {
		.v = {
			.codec_id = VIDEO_CODEC,
			.fps      = V1_FPS,
			.bps      = V1_BPS
		}
	}
};

static rtsp2_params_t rtsp2_a_pcmu_params = {
	.type = AVMEDIA_TYPE_AUDIO,
	.u = {
		.a = {
			.codec_id   = AV_CODEC_ID_PCMU,
			.channel    = 1,
			.samplerate = 8000
		}
	}
};

static rtp_params_t rtp_g711d_params = {
	.valid_pt = 0xFFFFFFFF,
	.port = 16384,
	.frame_size = 1500,
	.cache_depth = 6
};

static g711_params_t g711d_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_DECODE
};

static array_params_t doorbell_pcmu_array_params = {
	.type = AVMEDIA_TYPE_AUDIO,
	.codec_id = AV_CODEC_ID_PCMU,
	.mode = ARRAY_MODE_ONCE,
	.u = {
		.a = {
			.channel    = 1,
			.samplerate = 8000,
			.frame_size = 160,
		}
	}
};

void doorbell_ring(void)
{
	int state = 0;
	mm_module_ctrl(array_ctx, CMD_ARRAY_GET_STATE, (int)&state);
	if (state) {
		printf("doorbell is ringing\n\r");
	} else {
		printf("start doorbell_ring\n\r");
		miso_resume(miso_rtp_array_g711d);
		miso_pause(miso_rtp_array_g711d, MM_INPUT0);	// pause audio from rtp
		mm_module_ctrl(array_ctx, CMD_ARRAY_STREAMING, 1);	// doorbell ring

		do {	// wait until doorbell_ring done
			vTaskDelay(100);
			mm_module_ctrl(array_ctx, CMD_ARRAY_GET_STATE, (int)&state);
		} while (state == 1);

		miso_resume(miso_rtp_array_g711d);
		miso_pause(miso_rtp_array_g711d, MM_INPUT1);	// pause array
		printf("doorbell_ring done!\n\r");
	}
}

void mmf2_video_example_2way_audio_pcmu_doorbell_init(void)
{
	int voe_heap_size = video_voe_presetting(1, V1_WIDTH, V1_HEIGHT, V1_BPS, 0,
						0, 0, 0, 0, 0,
						0, 0, 0, 0, 0,
						0, 0, 0);

	printf("\r\n voe heap size = %d\r\n", voe_heap_size);

	// ------ Channel 1--------------
	video_v1_ctx = mm_module_open(&video_module);
	if (video_v1_ctx) {
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_SET_VOE_HEAP, voe_heap_size);
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_SET_PARAMS, (int)&video_v1_params);
		mm_module_ctrl(video_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_FPS);
		mm_module_ctrl(video_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(video_v1_ctx, CMD_VIDEO_APPLY, V1_CHANNEL);	// start channel 0
	} else {
		rt_printf("video open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	//--------------Audio --------------
	audio_ctx = mm_module_open(&audio_module);
	if (audio_ctx) {
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	} else {
		rt_printf("audio open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	g711e_ctx = mm_module_open(&g711_module);
	if (g711e_ctx) {
		mm_module_ctrl(g711e_ctx, CMD_G711_SET_PARAMS, (int)&g711e_params);
		mm_module_ctrl(g711e_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711e_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711e_ctx, CMD_G711_APPLY, 0);
	} else {
		rt_printf("G711 open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	//--------------RTSP---------------
	rtsp2_v1_ctx = mm_module_open(&rtsp2_module);
	if (rtsp2_v1_ctx) {
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v1_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);

		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 1);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_a_pcmu_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);

		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_STREAMMING, ON);
	} else {
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	//--------------Link---------------------------
	siso_audio_g711e = siso_create();
	if (siso_audio_g711e) {
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711e_ctx, 0);
		siso_start(siso_audio_g711e);
	} else {
		rt_printf("siso_audio_g711e open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	rt_printf("siso_audio_g711e started\n\r");

	miso_video_g711_rtsp = miso_create();
	if (miso_video_g711_rtsp) {
		miso_ctrl(miso_video_g711_rtsp, MMIC_CMD_ADD_INPUT0, (uint32_t)video_v1_ctx, 0);
		miso_ctrl(miso_video_g711_rtsp, MMIC_CMD_ADD_INPUT1, (uint32_t)g711e_ctx, 0);
		miso_ctrl(miso_video_g711_rtsp, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_v1_ctx, 0);
		miso_start(miso_video_g711_rtsp);
	} else {
		rt_printf("miso_h264_g711_rtsp fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}
	rt_printf("miso_h264_g711_rtsp started\n\r");

	// RTP audio
	rtp_ctx = mm_module_open(&rtp_module);
	if (rtp_ctx) {
		mm_module_ctrl(rtp_ctx, CMD_RTP_SET_PARAMS, (int)&rtp_g711d_params);
		mm_module_ctrl(rtp_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(rtp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(rtp_ctx, CMD_RTP_APPLY, 0);
		mm_module_ctrl(rtp_ctx, CMD_RTP_STREAMING, 1);	// streamming on
	} else {
		rt_printf("RTP open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	// Audio array input (doorbell)
	array_t array;
	array.data_addr = (uint32_t) doorbell_pcmu_sample;
	array.data_len = (uint32_t) doorbell_pcmu_sample_size;
	array_ctx = mm_module_open(&array_module);
	if (array_ctx) {
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_PARAMS, (int)&doorbell_pcmu_array_params);
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_ARRAY, (int)&array);
		mm_module_ctrl(array_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(array_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(array_ctx, CMD_ARRAY_APPLY, 0);
		//mm_module_ctrl(array_ctx, CMD_ARRAY_STREAMING, 1);	// streamming on
	} else {
		rt_printf("ARRAY open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	// G711D
	g711d_ctx = mm_module_open(&g711_module);
	if (g711d_ctx) {
		mm_module_ctrl(g711d_ctx, CMD_G711_SET_PARAMS, (int)&g711d_params);
		mm_module_ctrl(g711d_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711d_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711d_ctx, CMD_G711_APPLY, 0);
	} else {
		rt_printf("G711 open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	miso_rtp_array_g711d = miso_create();
	if (miso_rtp_array_g711d) {
		miso_ctrl(miso_rtp_array_g711d, MMIC_CMD_ADD_INPUT0, (uint32_t)rtp_ctx, 0);
		miso_ctrl(miso_rtp_array_g711d, MMIC_CMD_ADD_INPUT1, (uint32_t)array_ctx, 0);
		miso_ctrl(miso_rtp_array_g711d, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711d_ctx, 0);
		miso_start(miso_rtp_array_g711d);
	} else {
		rt_printf("miso_rtp_array_g711d open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	rt_printf("miso_rtp_array_g711d started\n\r");

	siso_g711d_audio = siso_create();
	if (siso_g711d_audio) {
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_INPUT, (uint32_t)g711d_ctx, 0);
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_g711d_audio);
	} else {
		rt_printf("siso_g711d_audio open fail\n\r");
		goto mmf2_video_example_two_way_audio_pcmu_fail;
	}

	rt_printf("siso_g711d_audio started\n\r");

	//printf("Delay 10s and then ring the doorbell\n\r");
	//vTaskDelay(10000);
	//doorbell_ring();


	return;
mmf2_video_example_two_way_audio_pcmu_fail:

	return;
}