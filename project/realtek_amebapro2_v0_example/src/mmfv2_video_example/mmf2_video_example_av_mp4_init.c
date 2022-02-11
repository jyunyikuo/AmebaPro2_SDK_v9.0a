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
#include "module_aac.h"
#include "module_mp4.h"
#include "mmf2_pro2_video_config.h"
#include "video_example_media_framework.h"

/*****************************************************************************
* ISP channel : 0
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
#include "sample_h265.h"
#define VIDEO_TYPE VIDEO_HEVC
#define VIDEO_CODEC AV_CODEC_ID_H265
#else
#include "sample_h264.h"
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

#define AAC_ENCODE_MODE
//#define G711_ULAW_MODE
//#define G711_ALAW_MODE

static mm_context_t *video_v1_ctx			= NULL;
static mm_context_t *audio_ctx				= NULL;
static mm_context_t *aac_ctx				= NULL;
static mm_context_t *mp4_ctx				= NULL;
static mm_context_t *g711e_ctx				= NULL;


static mm_siso_t *siso_audio_aac			= NULL;
static mm_siso_t *siso_audio_g711e			= NULL;
static mm_miso_t *miso_video_aac_mp4		= NULL;
static mm_miso_t *miso_video_g711e_mp4		= NULL;

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

static aac_params_t aac_params = {
	.sample_rate = 8000,
	.channel = 1,
	.bit_length = FAAC_INPUT_16BIT,
	.output_format = 1,
	.mpeg_version = MPEG4,
	.mem_total_size = 10 * 1024,
	.mem_block_size = 128,
	.mem_frame_size = 1024
};

static g711_params_t g711e_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_ENCODE
};

static mp4_params_t mp4_v1_params = {
	.fps            = V1_FPS,
	.gop            = V1_GOP,
	.width = V1_WIDTH,
	.height = V1_HEIGHT,
	.sample_rate = 8000,
	.channel = 1,

	.record_length = 10, //seconds
	.record_type = STORAGE_ALL,
	.record_file_num = 1,
	.record_file_name = "AmebaPro_recording",
	.fatfs_buf_size = 224 * 1024, /* 32kb multiple */
	.mp4_audio_format = AUDIO_AAC,//AUDIO_ULAW
	.mp4_audio_duration = 20,//audio duration 20ms for PCM
};

int mp4_stop_cb(void *parm)
{
	printf("Record stop\r\n");
}
int mp4_end_cb(void *parm)
{
	printf("Record end\r\n");
}

void mmf2_video_example_av_mp4_init(void)
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
		goto mmf2_video_exmaple_av_mp4_fail;
	}

	mp4_ctx = mm_module_open(&mp4_module);
#ifdef AAC_ENCODE_MODE
	mp4_v1_params.mp4_audio_format = AUDIO_AAC;
#endif

#ifdef G711_ULAW_MODE
	mp4_v1_params.mp4_audio_format = AUDIO_ULAW;
#endif

#ifdef G711_ALAW_MODE
	mp4_v1_params.mp4_audio_format = AUDIO_ALAW;
#endif
	if (mp4_ctx) {
		mm_module_ctrl(mp4_ctx, CMD_MP4_SET_PARAMS, (int)&mp4_v1_params);
		mm_module_ctrl(mp4_ctx, CMD_MP4_LOOP_MODE, 0);
		mm_module_ctrl(mp4_ctx, CMD_MP4_START, mp4_v1_params.record_file_num);
		mm_module_ctrl(mp4_ctx, CMD_MP4_SET_STOP_CB, (int)mp4_stop_cb);
		mm_module_ctrl(mp4_ctx, CMD_MP4_SET_END_CB, (int)mp4_end_cb);
	} else {
		rt_printf("MP4 open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}

	rt_printf("MP4 opened\n\r");

	audio_ctx = mm_module_open(&audio_module);
	if (audio_ctx) {
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	} else {
		rt_printf("AUDIO open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}
#ifdef AAC_ENCODE_MODE
	aac_ctx = mm_module_open(&aac_module);
	if (aac_ctx) {
		mm_module_ctrl(aac_ctx, CMD_AAC_SET_PARAMS, (int)&aac_params);
		mm_module_ctrl(aac_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(aac_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(aac_ctx, CMD_AAC_INIT_MEM_POOL, 0);
		mm_module_ctrl(aac_ctx, CMD_AAC_APPLY, 0);
	} else {
		rt_printf("AAC open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}
#else
	g711e_ctx = mm_module_open(&g711_module);
#ifdef G711_ULAW_MODE
	g711e_params.codec_id = AV_CODEC_ID_PCMU;
#endif

#ifdef G711_ALAW_MODE
	g711e_params.codec_id = AV_CODEC_ID_PCMA;
#endif
	if (g711e_ctx) {
		mm_module_ctrl(g711e_ctx, CMD_G711_SET_PARAMS, (int)&g711e_params);
		mm_module_ctrl(g711e_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711e_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711e_ctx, CMD_G711_APPLY, 0);
	} else {
		rt_printf("G711 open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}
#endif

#ifdef AAC_ENCODE_MODE
	siso_audio_aac = siso_create();
	if (siso_audio_aac) {
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_OUTPUT, (uint32_t)aac_ctx, 0);
		siso_start(siso_audio_aac);
	} else {
		rt_printf("siso1 open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}
#else
	siso_audio_g711e = siso_create();
	if (siso_audio_g711e) {
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711e_ctx, 0);
		siso_start(siso_audio_g711e);
	} else {
		rt_printf("siso_audio_g711e open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}
#endif

	rt_printf("siso1 started\n\r");

#ifdef AAC_ENCODE_MODE
	miso_video_aac_mp4 = miso_create();
	if (miso_video_aac_mp4) {
		miso_ctrl(miso_video_aac_mp4, MMIC_CMD_ADD_INPUT0, (uint32_t)video_v1_ctx, 0);
		miso_ctrl(miso_video_aac_mp4, MMIC_CMD_ADD_INPUT1, (uint32_t)aac_ctx, 0);
		miso_ctrl(miso_video_aac_mp4, MMIC_CMD_ADD_OUTPUT, (uint32_t)mp4_ctx, 0);
		miso_start(miso_video_aac_mp4);
	} else {
		rt_printf("miso open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}
#else
	miso_video_g711e_mp4 = miso_create();
	if (miso_video_g711e_mp4) {
		miso_ctrl(miso_video_g711e_mp4, MMIC_CMD_ADD_INPUT0, (uint32_t)video_v1_ctx, 0);
		miso_ctrl(miso_video_g711e_mp4, MMIC_CMD_ADD_INPUT1, (uint32_t)g711e_ctx, 0);
		miso_ctrl(miso_video_g711e_mp4, MMIC_CMD_ADD_OUTPUT, (uint32_t)mp4_ctx, 0);
		miso_start(miso_video_g711e_mp4);
	} else {
		rt_printf("miso open fail\n\r");
		goto mmf2_video_exmaple_av_mp4_fail;
	}
#endif
	rt_printf("miso started\n\r");

	return;
mmf2_video_exmaple_av_mp4_fail:

	return;
}