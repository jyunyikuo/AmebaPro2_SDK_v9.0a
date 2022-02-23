/******************************************************************************
*
* Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"

#include "module_array.h"
#include "module_audio.h"
#include "module_vipnn.h"
//#include "model_yamnet.h"
#include "model_yamnet_s.h"

#include "avcodec.h"
#include "wave_sample/wave_sample.h"
#include "input_wav_baby_975ms.h"
#include "input_wav_dog_975ms.h"
// TODO: move model id to proper header

//#define SIM_WAVE examples_dog_975ms_wav
//#define SIM_WAVE_LEN examples_dog_975ms_wav_len
//#define SIM_WAVE  examples_baby_975ms_wav
//#define SIM_WAVE_LEN examples_baby_975ms_wav_len
#define SIM_WAVE  sample5_wav
#define SIM_WAVE_LEN sample5_len

#define AUDIO_SIM	0

#if AUDIO_SIM
static mm_context_t *array_ctx          = NULL;
#else
static mm_context_t *audio_ctx				= NULL;
#endif

static mm_context_t *vipnn_ctx            = NULL;
static mm_siso_t *siso_audio_vipnn        = NULL;

#if AUDIO_SIM
static array_params_t wav_array_params = {
	.type = AVMEDIA_TYPE_AUDIO,
	.codec_id = AV_CODEC_ID_PCM_RAW,
	.mode = ARRAY_MODE_LOOP,
	.u = {
		.a = {
			.channel    = 1,
			.samplerate = 16000,
			.sample_bit_length = 16,
			.frame_size = 1024
		}
	}
};
#else
static audio_params_t audio_params = {
	.sample_rate = ASR_16KHZ,
	.word_length = WL_16BIT,
	.mic_gain    = MIC_40DB,
	.dmic_l_gain    = DMIC_BOOST_24DB,
	.dmic_r_gain    = DMIC_BOOST_24DB,
	.use_mic_type   = USE_AUDIO_AMIC,
	.channel     = 1,
	.enable_aec  = 0
};
#endif

static nn_data_param_t aud_info = {
	.aud = {
		.bit_pre_sample = 16,
		.channel = 1,
		.sample_rate = 16000
	}
};


void mmf2_video_example_audio_vipnn_init(void)
{
#if AUDIO_SIM
	array_t array;
	//array.data_addr = (uint32_t) examples_baby_975ms_wav;
	//array.data_len = (uint32_t) examples_baby_975ms_wav_len;
	array.data_addr = (uint32_t) SIM_WAVE;
	array.data_len = (uint32_t) SIM_WAVE_LEN;
	array_ctx = mm_module_open(&array_module);
	if (array_ctx) {
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_PARAMS, (int)&wav_array_params);
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_ARRAY, (int)&array);
		mm_module_ctrl(array_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(array_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(array_ctx, CMD_ARRAY_APPLY, 0);
		mm_module_ctrl(array_ctx, CMD_ARRAY_STREAMING, 1);	// streamming on
	} else {
		rt_printf("ARRAY open fail\n\r");
		goto mmf2_example_audio_vipnn_fail;
	}
#else
	//--------------Audio --------------
	audio_ctx = mm_module_open(&audio_module);
	if (audio_ctx) {
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	} else {
		rt_printf("audio open fail\n\r");
		goto mmf2_example_audio_vipnn_fail;
	}
#endif

	// YAMNET
	vipnn_ctx = mm_module_open(&vipnn_module);
	if (vipnn_ctx) {
		//mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&yamnet_fwfs);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&yamnet_s);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_IN_PARAMS, (int)&aud_info);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_DISPPOST, (int)0);

		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_APPLY, 0);
	} else {
		rt_printf("VIPNN open fail\n\r");
		goto mmf2_example_audio_vipnn_fail;
	}
	rt_printf("VIPNN opened\n\r");

	//--------------Link---------------------------
	siso_audio_vipnn = siso_create();
	if (siso_audio_vipnn) {
#if AUDIO_SIM
		siso_ctrl(siso_audio_vipnn, MMIC_CMD_ADD_INPUT, (uint32_t)array_ctx, 0);
#else
		siso_ctrl(siso_audio_vipnn, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
#endif
		siso_ctrl(siso_audio_vipnn, MMIC_CMD_ADD_OUTPUT, (uint32_t)vipnn_ctx, 0);
		siso_start(siso_audio_vipnn);
	} else {
		rt_printf("siso_audio_vipnn open fail\n\r");
		goto mmf2_example_audio_vipnn_fail;
	}
	rt_printf("siso_audio_vipnn started\n\r");

	return;
mmf2_example_audio_vipnn_fail:

	return;
}



