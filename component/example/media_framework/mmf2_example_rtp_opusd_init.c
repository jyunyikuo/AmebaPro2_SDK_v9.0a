/******************************************************************************
*
* Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
*
******************************************************************************/
//#include "example_media_framework.h"
#include "mmf2_link.h"
#include "mmf2_siso.h"

#include "module_audio.h"
#include "module_opusd.h"
#include "module_rtp.h"

static mm_context_t *rtp_ctx		= NULL;
static mm_context_t *opusd_ctx		= NULL;
static mm_context_t *audio_ctx		= NULL;

static mm_siso_t *siso_rtp_opusd    = NULL;
static mm_siso_t *siso_opusd_audio	= NULL;

static audio_params_t audio_params = {
#if defined(CONFIG_PLATFORM_8721D)
	.sample_rate = SR_8K,
	.word_length = WL_16,
	.mono_stereo = CH_MONO,
	// .direction = APP_AMIC_IN|APP_LINE_OUT,
	.direction = APP_LINE_IN | APP_LINE_OUT,
#else
	.sample_rate = ASR_8KHZ,
	.word_length = WL_16BIT,
	.mic_gain    = MIC_40DB,
	.dmic_l_gain    = DMIC_BOOST_24DB,
	.dmic_r_gain    = DMIC_BOOST_24DB,
	.use_mic_type   = USE_AUDIO_AMIC,
	.channel     = 1,
#endif
	.mix_mode = 0,
	.enable_aec  = 0
};

static opusd_params_t opusd_only_params = {
	.sample_rate = 8000,//16000,//
	.channel = 1,
	.bit_length = 16,         //16 recommand
	.frame_size_in_msec = 10, //will not be uused
	.with_opus_enc = 0,       //enable semaphore if the application with opus encoder
	.opus_application = OPUS_APPLICATION_AUDIO
};

static rtp_params_t rtp_opusd_params = {
	.valid_pt = 0xFFFFFFFF,
	.port = 16384,
	.frame_size = 1500,
	.cache_depth = 15
};


void mmf2_example_rtp_opusd_init(void)
{
	rtp_ctx = mm_module_open(&rtp_module);
	if (rtp_ctx) {
		mm_module_ctrl(rtp_ctx, CMD_RTP_SET_PARAMS, (int)&rtp_opusd_params);
		mm_module_ctrl(rtp_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(rtp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(rtp_ctx, CMD_RTP_APPLY, 0);
		mm_module_ctrl(rtp_ctx, CMD_RTP_STREAMING, 1);	// streamming on
	} else {
		rt_printf("RTP open fail\n\r");
		goto mmf2_exmaple_rtp_opusd_fail;
	}

	opusd_ctx = mm_module_open(&opusd_module);
	if (opusd_ctx) {
		mm_module_ctrl(opusd_ctx, CMD_OPUSD_SET_PARAMS, (int)&opusd_only_params);
		mm_module_ctrl(opusd_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(opusd_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(opusd_ctx, CMD_OPUSD_APPLY, 0);
	} else {
		rt_printf("OPUSD open fail\n\r");
		goto mmf2_exmaple_rtp_opusd_fail;
	}

	audio_ctx = mm_module_open(&audio_module);
	if (audio_ctx) {
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		//mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		//mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	} else {
		rt_printf("audio open fail\n\r");
		goto mmf2_exmaple_rtp_opusd_fail;
	}

	siso_rtp_opusd = siso_create();
	if (siso_rtp_opusd) {
		siso_ctrl(siso_rtp_opusd, MMIC_CMD_ADD_INPUT, (uint32_t)rtp_ctx, 0);
		siso_ctrl(siso_rtp_opusd, MMIC_CMD_ADD_OUTPUT, (uint32_t)opusd_ctx, 0);
		siso_ctrl(siso_rtp_opusd, MMIC_CMD_SET_STACKSIZE, 24 * 1024, 0);
		//siso_ctrl(siso_rtp_opusd, MMIC_CMD_SET_TASKNANE, (uint32_t)"rtp_opd", 0);
		siso_start(siso_rtp_opusd);
	} else {
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_rtp_opusd_fail;
	}

	rt_printf("siso3 started\n\r");

	siso_opusd_audio = siso_create();
	if (siso_opusd_audio) {
		siso_ctrl(siso_opusd_audio, MMIC_CMD_ADD_INPUT, (uint32_t)opusd_ctx, 0);
		siso_ctrl(siso_opusd_audio, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		//siso_ctrl(siso_opusd_audio, MMIC_CMD_SET_TASKNANE, (uint32_t)"opusd_audio", 0);
		siso_start(siso_opusd_audio);
	} else {
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_rtp_opusd_fail;
	}

	rt_printf("siso2 started\n\r");

	return;
mmf2_exmaple_rtp_opusd_fail:

	return;

}