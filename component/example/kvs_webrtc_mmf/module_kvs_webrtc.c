/******************************************************************************
*
* Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
*
******************************************************************************/

/* Headers for example */
#include "module_kvs_webrtc.h"
#include "AppMain.h"
#include "AppMediaSrc.h"

/* Config for Ameba-Pro */
#include "sample_config_webrtc.h"
#define STACK_SIZE              20*1024
#define KVS_QUEUE_DEPTH         20
#define WEBRTC_AUDIO_FRAME_SIZE 256

/* Network */
#include <lwip_netconf.h>
#include "wifi_conf.h"
#include <sntp/sntp.h>
#include "mbedtls/config.h"
uint8_t webrtc_wifi_ip[16];
uint8_t *ameba_get_ip(void)
{
	uint8_t *ip = LwIP_GetIP(0);
	memset(webrtc_wifi_ip, 0, sizeof(webrtc_wifi_ip) / sizeof(webrtc_wifi_ip[0]));
	memcpy(webrtc_wifi_ip, ip, 4);
	return webrtc_wifi_ip;
}

/* SD */
#include "vfs.h"

/* Audio/Video */
#include "avcodec.h"

xQueueHandle kvsWebrtcVideoSendQueue;
xQueueHandle kvsWebrtcAudioSendQueue;
xQueueHandle kvsWebrtcAudioRecvQueue;

static int ameba_platform_init(void)
{
#if defined(MBEDTLS_PLATFORM_C)
	mbedtls_platform_set_calloc_free(calloc, free);
#endif

	while (wifi_is_running(WLAN0_IDX) != 1) {
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
	printf("wifi connected\r\n");

	sntp_init();
	while (getEpochTimestampInHundredsOfNanos(NULL) < 10000000000000000ULL) {
		vTaskDelay(200 / portTICK_PERIOD_MS);
		printf("waiting get epoch timer\r\n");
	}

	vfs_init(NULL);
	vfs_user_register("sd", VFS_FATFS, VFS_INF_SD);

	return 0;
}

static void kvs_webrtc_main_thread(void *param)
{
	printf("=== KVS Example ===\n\r");

	// Ameba platform init
	if (ameba_platform_init() < 0) {
		printf("platform init fail\n\r");
		goto platform_init_fail;
	}

	WebRTCAppMain();

platform_init_fail:
	vfs_user_unregister("sd", VFS_FATFS, VFS_INF_SD);
	vfs_deinit(NULL);

	vTaskDelete(NULL);
}

#ifdef ENABLE_AUDIO_SENDRECV
static void kvs_webrtc_audio_thread(void *param)
{
	kvs_webrtc_ctx_t *ctx = (kvs_webrtc_ctx_t *)param;
	webrtc_audio_buf_t audio_rev_buf;

	while (1) {
		if (xQueueReceive(kvsWebrtcAudioRecvQueue, &audio_rev_buf, 0xFFFFFFFF) != pdTRUE) {
			continue;    // should not happen
		}

		mm_context_t *mctx = (mm_context_t *)ctx->parent;
		mm_queue_item_t *output_item;
		if (xQueueReceive(mctx->output_recycle, &output_item, 0xFFFFFFFF) == pdTRUE) {
			memcpy((void *)output_item->data_addr, (void *)audio_rev_buf.data_buf, audio_rev_buf.size);
			output_item->size = audio_rev_buf.size;
			output_item->type = audio_rev_buf.type;
			output_item->timestamp = audio_rev_buf.timestamp;
			xQueueSend(mctx->output_ready, (void *)&output_item, 0xFFFFFFFF);
			free(audio_rev_buf.data_buf);
		}
	}
}
#endif /* ENABLE_AUDIO_SENDRECV */

int kvs_webrtc_handle(void *p, void *input, void *output)
{
	kvs_webrtc_ctx_t *ctx = (kvs_webrtc_ctx_t *)p;

	mm_queue_item_t *input_item = (mm_queue_item_t *)input;

	if (input_item->type == AV_CODEC_ID_H264) {
		webrtc_video_buf_t video_buf;

		video_buf.output_size = input_item->size;
		video_buf.output_buffer_size = video_buf.output_size;
		video_buf.output_buffer = malloc(video_buf.output_size);

		memcpy(video_buf.output_buffer, (uint8_t *)input_item->data_addr, video_buf.output_size);

		video_buf.timestamp = xTaskGetTickCount();

		if (uxQueueSpacesAvailable(kvsWebrtcVideoSendQueue) != 0) {
			xQueueSend(kvsWebrtcVideoSendQueue, &video_buf, 0);
		} else {
			free(video_buf.output_buffer);
		}
	} else if ((input_item->type == AV_CODEC_ID_PCMU) || (input_item->type == AV_CODEC_ID_PCMA) || (input_item->type == AV_CODEC_ID_OPUS)) {
		webrtc_audio_buf_t audio_buf;
		audio_buf.size = input_item->size;

		audio_buf.data_buf =  malloc(audio_buf.size);
		memcpy(audio_buf.data_buf, (uint8_t *)input_item->data_addr, audio_buf.size);

		audio_buf.timestamp = input_item->timestamp;

		if (uxQueueSpacesAvailable(kvsWebrtcAudioSendQueue) != 0) {
			xQueueSend(kvsWebrtcAudioSendQueue, &audio_buf, 0);
		} else {
			free(audio_buf.data_buf);
		}
	}

	return 0;
}

int kvs_webrtc_control(void *p, int cmd, int arg)
{
	kvs_webrtc_ctx_t *ctx = (kvs_webrtc_ctx_t *)p;

	switch (cmd) {

	case CMD_KVS_WEBRTC_SET_APPLY:
		if (xTaskCreate(kvs_webrtc_main_thread, ((const char *)"kvs_webrtc_main_thread"), STACK_SIZE, NULL, tskIDLE_PRIORITY + 1,
						&ctx->kvs_webrtc_module_main_task) != pdPASS) {
			printf("\n\r%s xTaskCreate(kvs_webrtc_main_thread) failed", __FUNCTION__);
		}
#ifdef ENABLE_AUDIO_SENDRECV
		if (xTaskCreate(kvs_webrtc_audio_thread, ((const char *)"kvs_webrtc_audio_thread"), 512, (void *)ctx, tskIDLE_PRIORITY + 1,
						&ctx->kvs_webrtc_module_audio_recv_task) != pdPASS) {
			printf("\n\r%s xTaskCreate(kvs_webrtc_audio_thread) failed", __FUNCTION__);
		}
#endif
		break;
	}
	return 0;
}

void *kvs_webrtc_destroy(void *p)
{
	kvs_webrtc_ctx_t *ctx = (kvs_webrtc_ctx_t *)p;

	if (ctx && ctx->kvs_webrtc_module_main_task) {
		vTaskDelete(ctx->kvs_webrtc_module_main_task);
	}
	if (ctx && ctx->kvs_webrtc_module_audio_recv_task) {
		vTaskDelete(ctx->kvs_webrtc_module_audio_recv_task);
	}
	if (ctx) {
		free(ctx);
	}
	vQueueDelete(kvsWebrtcVideoSendQueue);
	vQueueDelete(kvsWebrtcAudioSendQueue);
#if ( defined(ENABLE_AUDIO_SENDRECV) && ( AUDIO_G711_MULAW || AUDIO_G711_ALAW ) )
	vQueueDelete(kvsWebrtcAudioRecvQueue);
#endif

	return NULL;
}

void *kvs_webrtc_create(void *parent)
{
	kvs_webrtc_ctx_t *ctx = malloc(sizeof(kvs_webrtc_ctx_t));
	if (!ctx) {
		return NULL;
	}
	memset(ctx, 0, sizeof(kvs_webrtc_ctx_t));
	ctx->parent = parent;

	kvsWebrtcVideoSendQueue = xQueueCreate(KVS_QUEUE_DEPTH, sizeof(webrtc_video_buf_t));
	xQueueReset(kvsWebrtcVideoSendQueue);

	kvsWebrtcAudioSendQueue = xQueueCreate(KVS_QUEUE_DEPTH * 3, sizeof(webrtc_audio_buf_t));
	xQueueReset(kvsWebrtcAudioSendQueue);

#if ( defined(ENABLE_AUDIO_SENDRECV) && ( AUDIO_G711_MULAW || AUDIO_G711_ALAW ) )
	//Create a queue to receive the G711 audio frame from viewer
	kvsWebrtcAudioRecvQueue = xQueueCreate(KVS_QUEUE_DEPTH * 6, sizeof(webrtc_audio_buf_t));
	xQueueReset(kvsWebrtcAudioRecvQueue);
#endif

	printf("kvs_webrtc_create...\r\n");

	return ctx;
}

void *kvs_webrtc_new_item(void *p)
{
	kvs_webrtc_ctx_t *ctx = (kvs_webrtc_ctx_t *)p;

	return (void *)malloc(WEBRTC_AUDIO_FRAME_SIZE * 2);
}

void *kvs_webrtc_del_item(void *p, void *d)
{
	(void)p;
	if (d) {
		free(d);
	}
	return NULL;
}

mm_module_t kvs_webrtc_module = {
	.create = kvs_webrtc_create,
	.destroy = kvs_webrtc_destroy,
	.control = kvs_webrtc_control,
	.handle = kvs_webrtc_handle,

	.new_item = kvs_webrtc_new_item,
	.del_item = kvs_webrtc_del_item,

	.output_type = MM_TYPE_NONE,        // output for video sink
	.module_type = MM_TYPE_AVSINK,      // module type is video algorithm
	.name = "KVS_WebRTC"
};
