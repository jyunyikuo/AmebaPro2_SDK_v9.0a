#include "platform_opts.h"
#if !CONFIG_EXAMPLE_KVS_PRODUCER

/* Headers for example */
#include "sample_config.h"
#include "module_kvs_producer.h"

#include "wifi_conf.h"

/* Headers for video */
#include "avcodec.h"

/* Headers for KVS */
#include "kvs/port.h"
#include "kvs/nalu.h"
#include "kvs/restapi.h"
#include "kvs/stream.h"

#include "mbedtls/config.h"

#define ERRNO_NONE      0
#define ERRNO_FAIL      __LINE__

static int kvsProducerModule_video_H;
static int kvsProducerModule_video_W;
static int start_recording = 0, initCamera_done = 0, initAudio_done = 0;
static Kvs_t xKvs = {0};

typedef struct {
	uint8_t *output_buffer;
	uint32_t output_buffer_size;
	uint32_t output_size;
} VIDEO_BUFFER;

int KvsVideoInitTrackInfo(VIDEO_BUFFER *pVideoBuf, Kvs_t *pKvs)
{
	int res = ERRNO_NONE;
	uint8_t *pVideoCpdData = NULL;
	uint32_t uCpdLen = 0;

	if (pVideoBuf == NULL || pKvs == NULL) {
		res = ERRNO_FAIL;
	} else if (pKvs->pVideoTrackInfo != NULL) {
		res = ERRNO_FAIL;
	} else if (Mkv_generateH264CodecPrivateDataFromAnnexBNalus(pVideoBuf->output_buffer, pVideoBuf->output_size, &pVideoCpdData, &uCpdLen) != 0) {
		res = ERRNO_FAIL;
	} else if ((pKvs->pVideoTrackInfo = (VideoTrackInfo_t *)malloc(sizeof(VideoTrackInfo_t))) == NULL) {
		res = ERRNO_FAIL;
	} else {
		memset(pKvs->pVideoTrackInfo, 0, sizeof(VideoTrackInfo_t));

		pKvs->pVideoTrackInfo->pTrackName = VIDEO_NAME;
		pKvs->pVideoTrackInfo->pCodecName = VIDEO_CODEC_NAME;
		pKvs->pVideoTrackInfo->uWidth = kvsProducerModule_video_W;
		pKvs->pVideoTrackInfo->uHeight = kvsProducerModule_video_H;
		pKvs->pVideoTrackInfo->pCodecPrivate = pVideoCpdData;
		pKvs->pVideoTrackInfo->uCodecPrivateLen = uCpdLen;
		printf("\r[Video resolution] w: %d h: %d\r\n", pKvs->pVideoTrackInfo->uWidth, pKvs->pVideoTrackInfo->uHeight);
	}

	return res;
}

static void sendVideoFrame(VIDEO_BUFFER *pBuffer, Kvs_t *pKvs)
{
	int res = ERRNO_NONE;
	DataFrameIn_t xDataFrameIn = {0};
	uint32_t uAvccLen = 0;
	uint8_t *pVideoCpdData = NULL;
	uint32_t uCpdLen = 0;
	size_t uMemTotal = 0;

	if (pBuffer == NULL || pKvs == NULL) {
		res = ERRNO_FAIL;
	} else {
		if (pKvs->xStreamHandle != NULL) {
			xDataFrameIn.bIsKeyFrame = isKeyFrame(pBuffer->output_buffer, pBuffer->output_size) ? true : false;
			xDataFrameIn.uTimestampMs = getEpochTimestampInMs();
			xDataFrameIn.xTrackType = TRACK_VIDEO;

			xDataFrameIn.xClusterType = (xDataFrameIn.bIsKeyFrame) ? MKV_CLUSTER : MKV_SIMPLE_BLOCK;

			if (NALU_convertAnnexBToAvccInPlace(pBuffer->output_buffer, pBuffer->output_size, pBuffer->output_buffer_size, &uAvccLen) != ERRNO_NONE) {
				printf("Failed to convert Annex-B to AVCC\r\n");
				res = ERRNO_FAIL;
			} else if ((xDataFrameIn.pData = (char *)malloc(uAvccLen)) == NULL) {
				printf("OOM: xDataFrameIn.pData\r\n");
				res = ERRNO_FAIL;
			} else if (Kvs_streamMemStatTotal(pKvs->xStreamHandle, &uMemTotal) != 0) {
				printf("Failed to get stream mem state\r\n");
			} else {
				if (uMemTotal < STREAM_MAX_BUFFERING_SIZE) {
					memcpy(xDataFrameIn.pData, pBuffer->output_buffer, uAvccLen);
					xDataFrameIn.uDataLen = uAvccLen;

					Kvs_streamAddDataFrame(pKvs->xStreamHandle, &xDataFrameIn);
				} else {
					free(xDataFrameIn.pData);
				}
			}
		}
		free(pBuffer->output_buffer);
	}
}

#if ENABLE_AUDIO_TRACK
int KvsAudioInitTrackInfo(Kvs_t *pKvs)
{
	int res = ERRNO_NONE;
	uint8_t *pCodecPrivateData = NULL;
	uint32_t uCodecPrivateDataLen = 0;

	pKvs->pAudioTrackInfo = (AudioTrackInfo_t *)malloc(sizeof(AudioTrackInfo_t));
	memset(pKvs->pAudioTrackInfo, 0, sizeof(AudioTrackInfo_t));
	pKvs->pAudioTrackInfo->pTrackName = AUDIO_NAME;
	pKvs->pAudioTrackInfo->pCodecName = AUDIO_CODEC_NAME;
	pKvs->pAudioTrackInfo->uFrequency = AUDIO_SAMPLING_RATE;
	pKvs->pAudioTrackInfo->uChannelNumber = AUDIO_CHANNEL_NUMBER;

#if USE_AUDIO_AAC
	res = Mkv_generateAacCodecPrivateData(AUDIO_MPEG_OBJECT_TYPE, AUDIO_SAMPLING_RATE, AUDIO_CHANNEL_NUMBER, &pCodecPrivateData, &uCodecPrivateDataLen);
#endif
#if USE_AUDIO_G711
	res = Mkv_generatePcmCodecPrivateData(AUDIO_PCM_OBJECT_TYPE, AUDIO_SAMPLING_RATE, AUDIO_CHANNEL_NUMBER, &pCodecPrivateData, &uCodecPrivateDataLen);
#endif
	if (res == ERRNO_NONE) {
		pKvs->pAudioTrackInfo->pCodecPrivate = pCodecPrivateData;
		pKvs->pAudioTrackInfo->uCodecPrivateLen = (uint32_t)uCodecPrivateDataLen;
	} else {
		printf("Failed to generate codec private data\r\n");
	}
}
#endif

static int kvsInitialize(Kvs_t *pKvs)
{
	int res = ERRNO_NONE;
	char *pcStreamName = KVS_STREAM_NAME;

	if (pKvs == NULL) {
		res = ERRNO_FAIL;
	} else {
		memset(pKvs, 0, sizeof(Kvs_t));

		pKvs->xServicePara.pcHost = AWS_KVS_HOST;
		pKvs->xServicePara.pcRegion = AWS_KVS_REGION;
		pKvs->xServicePara.pcService = AWS_KVS_SERVICE;
		pKvs->xServicePara.pcAccessKey = AWS_ACCESS_KEY;
		pKvs->xServicePara.pcSecretKey = AWS_SECRET_KEY;

		pKvs->xDescPara.pcStreamName = pcStreamName;

		pKvs->xCreatePara.pcStreamName = pcStreamName;
		pKvs->xCreatePara.uDataRetentionInHours = 2;

		pKvs->xGetDataEpPara.pcStreamName = pcStreamName;

		pKvs->xPutMediaPara.pcStreamName = pcStreamName;
		pKvs->xPutMediaPara.xTimecodeType = TIMECODE_TYPE_ABSOLUTE;

#if ENABLE_IOT_CREDENTIAL
		pKvs->xIotCredentialReq.pCredentialHost = CREDENTIALS_HOST;
		pKvs->xIotCredentialReq.pRoleAlias = ROLE_ALIAS;
		pKvs->xIotCredentialReq.pThingName = THING_NAME;
		pKvs->xIotCredentialReq.pRootCA = ROOT_CA;
		pKvs->xIotCredentialReq.pCertificate = CERTIFICATE;
		pKvs->xIotCredentialReq.pPrivateKey = PRIVATE_KEY;
#endif
	}
	return res;
}

static void kvsTerminate(Kvs_t *pKvs)
{
	if (pKvs != NULL) {
		if (pKvs->xServicePara.pcPutMediaEndpoint != NULL) {
			free(pKvs->xServicePara.pcPutMediaEndpoint);
			pKvs->xServicePara.pcPutMediaEndpoint = NULL;
		}
	}
}

static int setupDataEndpoint(Kvs_t *pKvs)
{
	int res = ERRNO_NONE;
	unsigned int uHttpStatusCode = 0;

	if (pKvs == NULL) {
		res = ERRNO_FAIL;
	} else {
		if (pKvs->xServicePara.pcPutMediaEndpoint != NULL) {
		} else {
			printf("Try to describe stream\r\n");
			if (Kvs_describeStream(&(pKvs->xServicePara), &(pKvs->xDescPara), &uHttpStatusCode) != 0 || uHttpStatusCode != 200) {
				printf("Failed to describe stream\r\n");
				printf("Try to create stream\r\n");
				if (Kvs_createStream(&(pKvs->xServicePara), &(pKvs->xCreatePara), &uHttpStatusCode) != 0 || uHttpStatusCode != 200) {
					printf("uHttpStatusCode != 200 = %d\r\n", uHttpStatusCode);
					printf("Failed to create stream\r\n");
					res = ERRNO_FAIL;
				}
			}

			if (res == ERRNO_NONE) {
				if (Kvs_getDataEndpoint(&(pKvs->xServicePara), &(pKvs->xGetDataEpPara), &uHttpStatusCode, &(pKvs->xServicePara.pcPutMediaEndpoint)) != 0 ||
					uHttpStatusCode != 200) {
					printf("Failed to get data endpoint\r\n");
					res = ERRNO_FAIL;
				}
			}
		}
	}

	if (res == ERRNO_NONE) {
		printf("PUT MEDIA endpoint: %s\r\n", pKvs->xServicePara.pcPutMediaEndpoint);
	}

	return res;
}

static void streamFlush(StreamHandle xStreamHandle)
{
	DataFrameHandle xDataFrameHandle = NULL;
	DataFrameIn_t *pDataFrameIn = NULL;

	while ((xDataFrameHandle = Kvs_streamPop(xStreamHandle)) != NULL) {
		pDataFrameIn = (DataFrameIn_t *)xDataFrameHandle;
		free(pDataFrameIn->pData);
		Kvs_dataFrameTerminate(xDataFrameHandle);
	}
}

static void streamFlushToNextCluster(StreamHandle xStreamHandle)
{
	DataFrameHandle xDataFrameHandle = NULL;
	DataFrameIn_t *pDataFrameIn = NULL;

	while (1) {
		xDataFrameHandle = Kvs_streamPeek(xStreamHandle);
		if (xDataFrameHandle == NULL) {
			sleepInMs(50);
		} else {
			pDataFrameIn = (DataFrameIn_t *)xDataFrameHandle;
			if (pDataFrameIn->xClusterType == MKV_CLUSTER) {
				break;
			} else {
				xDataFrameHandle = Kvs_streamPop(xStreamHandle);
				pDataFrameIn = (DataFrameIn_t *)xDataFrameHandle;
				free(pDataFrameIn->pData);
				Kvs_dataFrameTerminate(xDataFrameHandle);
			}
		}
	}
}

static int putMediaSendData(Kvs_t *pKvs, int *pxSendCnt)
{
	int res = 0;
	DataFrameHandle xDataFrameHandle = NULL;
	DataFrameIn_t *pDataFrameIn = NULL;
	uint8_t *pData = NULL;
	size_t uDataLen = 0;
	uint8_t *pMkvHeader = NULL;
	size_t uMkvHeaderLen = 0;
	int xSendCnt = 0;

	if (Kvs_streamAvailOnTrack(pKvs->xStreamHandle, TRACK_VIDEO)
#if ENABLE_AUDIO_TRACK
		&& Kvs_streamAvailOnTrack(pKvs->xStreamHandle, TRACK_AUDIO)
#endif
	   ) {
		if ((xDataFrameHandle = Kvs_streamPop(pKvs->xStreamHandle)) == NULL) {
			printf("Failed to get data frame\r\n");
			res = ERRNO_FAIL;
		} else if (Kvs_dataFrameGetContent(xDataFrameHandle, &pMkvHeader, &uMkvHeaderLen, &pData, &uDataLen) != 0) {
			printf("Failed to get data and mkv header to send\r\n");
			res = ERRNO_FAIL;
		} else if (Kvs_putMediaUpdate(pKvs->xPutMediaHandle, pMkvHeader, uMkvHeaderLen, pData, uDataLen) != 0) {
			printf("Failed to update\r\n");
			res = ERRNO_FAIL;
		} else {
			xSendCnt++;
		}

		if (xDataFrameHandle != NULL) {
			pDataFrameIn = (DataFrameIn_t *)xDataFrameHandle;
			free(pDataFrameIn->pData);
			Kvs_dataFrameTerminate(xDataFrameHandle);
		}
	}

	if (pxSendCnt != NULL) {
		*pxSendCnt = xSendCnt;
	}

	return res;
}

static int putMedia(Kvs_t *pKvs)
{
	int res = 0;
	unsigned int uHttpStatusCode = 0;
	uint8_t *pEbmlSeg = NULL;
	size_t uEbmlSegLen = 0;
	int xSendCnt = 0;

	printf("Try to put media\r\n");
	if (pKvs == NULL) {
		printf("Invalid argument: pKvs\r\n");
		res = ERRNO_FAIL;
	} else if (Kvs_putMediaStart(&(pKvs->xServicePara), &(pKvs->xPutMediaPara), &uHttpStatusCode, &(pKvs->xPutMediaHandle)) != 0 || uHttpStatusCode != 200) {
		printf("Failed to setup PUT MEDIA\r\n");
		res = ERRNO_FAIL;
	} else if (Kvs_streamGetMkvEbmlSegHdr(pKvs->xStreamHandle, &pEbmlSeg, &uEbmlSegLen) != 0 ||
			   Kvs_putMediaUpdateRaw(pKvs->xPutMediaHandle, pEbmlSeg, uEbmlSegLen) != 0) {
		printf("Failed to upadte MKV EBML and segment\r\n");
		res = ERRNO_FAIL;
	} else {
		/* The beginning of a KVS stream has to be a cluster frame. */
		streamFlushToNextCluster(pKvs->xStreamHandle);

		while (1) {
			if (putMediaSendData(pKvs, &xSendCnt) != ERRNO_NONE) {
				break;
			}
			if (Kvs_putMediaDoWork(pKvs->xPutMediaHandle) != ERRNO_NONE) {
				break;
			}
			if (xSendCnt == 0) {
				sleepInMs(50);
			}
		}
	}

	printf("Leaving put media\r\n");
	Kvs_putMediaFinish(pKvs->xPutMediaHandle);
	pKvs->xPutMediaHandle = NULL;

	return res;
}

static int initCamera(Kvs_t *pKvs)
{
	int res = ERRNO_NONE;

	initCamera_done = 1;

	while (pKvs->pVideoTrackInfo == NULL) {
		sleepInMs(50);
	}
	return res;
}

#if ENABLE_AUDIO_TRACK
static int initAudio(Kvs_t *pKvs)
{
	int res = ERRNO_NONE;

	KvsAudioInitTrackInfo(pKvs); // Initialize audio track info

	while (pKvs->pAudioTrackInfo == NULL) {
		sleepInMs(50);
	}

	initAudio_done = 1;

	return res;
}
#endif /* ENABLE_AUDIO_TRACK */

void Kvs_run(Kvs_t *pKvs)
{
	int res = ERRNO_NONE;
	unsigned int uHttpStatusCode = 0;

#if ENABLE_IOT_CREDENTIAL
	IotCredentialToken_t *pToken = NULL;
#endif /* ENABLE_IOT_CREDENTIAL */
	if (kvsInitialize(pKvs) != 0) {
		printf("Failed to initialize KVS\r\n");
		res = ERRNO_FAIL;
	} else if (initCamera(pKvs) != 0) {
		printf("Failed to init camera\r\n");
		res = ERRNO_FAIL;
	}
#if ENABLE_AUDIO_TRACK
	else if (initAudio(pKvs) != 0) {
		printf("Failed to init audio\r\n");
		res = ERRNO_FAIL;
	}
#endif /* ENABLE_AUDIO_TRACK */
	else if ((pKvs->xStreamHandle = Kvs_streamCreate(pKvs->pVideoTrackInfo, pKvs->pAudioTrackInfo)) == NULL) {
		printf("Failed to create stream\r\n");
		res = ERRNO_FAIL;
	} else {
		while (1) {
#if ENABLE_IOT_CREDENTIAL
			Iot_credentialTerminate(pToken);
			if ((pToken = Iot_getCredential(&(pKvs->xIotCredentialReq))) == NULL) {
				printf("Failed to get Iot credential\r\n");
				sleepInMs(100);
				continue; //break;
			} else {
				pKvs->xServicePara.pcAccessKey = pToken->pAccessKeyId;
				pKvs->xServicePara.pcSecretKey = pToken->pSecretAccessKey;
				pKvs->xServicePara.pcToken = pToken->pSessionToken;
			}
#endif
			if (setupDataEndpoint(pKvs) != ERRNO_NONE) {
				printf("Failed to get PUT MEDIA endpoint");
			} else if (putMedia(pKvs) != ERRNO_NONE) {
				printf("End of PUT MEDIA\r\n");
				break;
			}
			sleepInMs(100); /* Wait for retry */
		}
	}
}

static void ameba_platform_init(void)
{
#if defined(MBEDTLS_PLATFORM_C)
	mbedtls_platform_set_calloc_free(calloc, free);
#endif

	while (wifi_is_running(WLAN0_IDX) != 1) {
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
	printf("wifi connected\r\n");

	sntp_init();
	while (getEpochTimestampInMs() < 100000000ULL) {
		vTaskDelay(200 / portTICK_PERIOD_MS);
		printf("waiting get epoch timer\r\n");
	}
}

static void kvs_producer_thread(void *param)
{
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	// Ameba platform init
	ameba_platform_init();

	// kvs produccer init
	platformInit();

	Kvs_run(&xKvs);

	vTaskDelete(NULL);
}

int kvs_producer_handle(void *p, void *input, void *output)
{
	kvs_producer_ctx_t *ctx = (kvs_producer_ctx_t *)p;
	mm_queue_item_t *input_item = (mm_queue_item_t *)input;

	VIDEO_BUFFER video_buf;
	Kvs_t *pKvs = &xKvs;

	if (input_item->type == AV_CODEC_ID_H264) {
		if (initCamera_done) {
			video_buf.output_size = input_item->size;
			video_buf.output_buffer_size = video_buf.output_size;
			video_buf.output_buffer = malloc(video_buf.output_size);
			memcpy(video_buf.output_buffer, (uint8_t *)input_item->data_addr, video_buf.output_size);

			if (start_recording) {
				sendVideoFrame(&video_buf, pKvs);
			} else {
				if (isKeyFrame(video_buf.output_buffer, video_buf.output_size)) {
					start_recording = 1;
					if (pKvs->pVideoTrackInfo == NULL) {
						KvsVideoInitTrackInfo(&video_buf, pKvs);
					}
					sendVideoFrame(&video_buf, pKvs);
				} else {
					if (video_buf.output_buffer != NULL) {
						free(video_buf.output_buffer);
					}
				}
			}
		}
	}
#if ENABLE_AUDIO_TRACK
	else if (input_item->type == AV_CODEC_ID_MP4A_LATM || input_item->type == AV_CODEC_ID_PCMU || input_item->type == AV_CODEC_ID_PCMA) {
		if (initAudio_done) {
			uint8_t *pAudioFrame = NULL;
			pAudioFrame = (uint8_t *)malloc(input_item->size);
			memcpy(pAudioFrame, (uint8_t *)input_item->data_addr, input_item->size);

			DataFrameIn_t xDataFrameIn = {0};
			memset(&xDataFrameIn, 0, sizeof(DataFrameIn_t));
			xDataFrameIn.bIsKeyFrame = false;
			xDataFrameIn.uTimestampMs = getEpochTimestampInMs();
			xDataFrameIn.xTrackType = TRACK_AUDIO;

			xDataFrameIn.xClusterType = MKV_SIMPLE_BLOCK;

			xDataFrameIn.pData = (char *)pAudioFrame;
			xDataFrameIn.uDataLen = input_item->size;

			size_t uMemTotal = 0;
			if (Kvs_streamMemStatTotal(pKvs->xStreamHandle, &uMemTotal) != 0) {
				printf("Failed to get stream mem state\r\n");
			} else if ((pKvs->xStreamHandle != NULL) && (uMemTotal < STREAM_MAX_BUFFERING_SIZE)) {
				Kvs_streamAddDataFrame(pKvs->xStreamHandle, &xDataFrameIn);
			} else {
				free(xDataFrameIn.pData);
			}
		}
	}
#endif

	return 0;
}

int kvs_producer_control(void *p, int cmd, int arg)
{
	kvs_producer_ctx_t *ctx = (kvs_producer_ctx_t *)p;

	switch (cmd) {

	case CMD_KVS_PRODUCER_SET_APPLY:
		if (xTaskCreate(kvs_producer_thread, ((const char *)"kvs_producer_thread"), 4096, NULL, tskIDLE_PRIORITY + 1, &ctx->kvs_producer_module_task) != pdPASS) {
			printf("\n\r%s xTaskCreate(kvs_producer_thread) failed", __FUNCTION__);
		}
		break;
	case CMD_KVS_PRODUCER_SET_VIDEO_HEIGHT:
		kvsProducerModule_video_H = (int)arg;
		break;
	case CMD_KVS_PRODUCER_SET_VIDEO_WIDTH:
		kvsProducerModule_video_W = (int)arg;
		break;
	}
	return 0;
}

void *kvs_producer_destroy(void *p)
{
	kvs_producer_ctx_t *ctx = (kvs_producer_ctx_t *)p;

	if (ctx) {
		free(ctx);
	}
	if (ctx && ctx->kvs_producer_module_task) {
		vTaskDelete(ctx->kvs_producer_module_task);
	}
	start_recording = 0;
	initCamera_done = 0;
	initAudio_done = 0;

	return NULL;
}

void *kvs_producer_create(void *parent)
{
	kvs_producer_ctx_t *ctx = malloc(sizeof(kvs_producer_ctx_t));
	if (!ctx) {
		return NULL;
	}
	memset(ctx, 0, sizeof(kvs_producer_ctx_t));
	ctx->parent = parent;

	printf("kvs_producer_create...\r\n");

	return ctx;
}

mm_module_t kvs_producer_module = {
	.create = kvs_producer_create,
	.destroy = kvs_producer_destroy,
	.control = kvs_producer_control,
	.handle = kvs_producer_handle,

	.new_item = NULL,
	.del_item = NULL,

	.output_type = MM_TYPE_NONE,    // output for video sink
	.module_type = MM_TYPE_VDSP,    // module type is video algorithm
	.name = "KVS_Producer"
};

#endif
