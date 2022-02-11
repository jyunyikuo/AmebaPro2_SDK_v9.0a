/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#define LOG_CLASS "AppFileSrc"
#include "AppMediaSrc.h"
#include "AppCommon.h"
#include "fileio.h"

#include "../sample_config_webrtc.h"
#include "avcodec.h"

extern xQueueHandle kvsWebrtcVideoSendQueue;
extern xQueueHandle kvsWebrtcAudioSendQueue;
extern xQueueHandle kvsWebrtcAudioRecvQueue;

/* used to monitor skb resource */
extern int skbbuf_used_num;
extern int skbdata_used_num;
extern int max_local_skb_num;
extern int max_skb_buf_num;

static int h264_is_i_frame(u8 *frame_buf)
{
	if ((frame_buf[4] & 0x1F) == 7) {
		return 1;
	} else {
		return 0;
	}
}

typedef struct {
	RTC_CODEC codec;
	PBYTE pFrameBuffer;
	UINT32 frameBufferSize;
} CodecStreamConf, *PCodecStreamConf;

typedef struct {
	STATUS codecStatus;
	CodecStreamConf videoStream;
	CodecStreamConf audioStream;
} CodecConfiguration, *PCodecConfiguration;

typedef struct {
	MUTEX codecConfLock;
	CodecConfiguration codecConfiguration;  //!< the configuration of gstreamer.
	// the codec.
	volatile ATOMIC_BOOL shutdownFileSrc;
	volatile ATOMIC_BOOL codecConfigLatched;
	// for meida output.
	PVOID mediaSinkHookUserdata;
	MediaSinkHook mediaSinkHook;
	PVOID mediaEosHookUserdata;
	MediaEosHook mediaEosHook;

} FileSrcContext, *PFileSrcContext;

PVOID sendVideoPackets(PVOID args)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) args;
	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);
	Frame frame;
	UINT32 frameSize;
	STATUS status;
	frame.presentationTs = 0;

	u8 start_transfer = 0;
	webrtc_video_buf_t video_buf;

	while (!ATOMIC_LOAD_BOOL(&pFileSrcContext->shutdownFileSrc)) {
		if (xQueueReceive(kvsWebrtcVideoSendQueue, &video_buf, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		// #TBD
		frame.flags = h264_is_i_frame(video_buf.output_buffer) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
		frame.frameData = video_buf.output_buffer;
		frame.size = video_buf.output_size;
		frame.presentationTs = getEpochTimestampInHundredsOfNanos(&video_buf.timestamp);

		frame.trackId = DEFAULT_VIDEO_TRACK_ID;
		//frame.duration = 0;
		frame.version = FRAME_CURRENT_VERSION;
		frame.decodingTs = frame.presentationTs;

		/* wait for skb resource release */
		if ((skbdata_used_num > (max_skb_buf_num - 64)) || (skbbuf_used_num > (max_local_skb_num - 64))) {
			if (video_buf.output_buffer != NULL) {
				SAFE_MEMFREE(video_buf.output_buffer);
			}
			continue; //skip this frame and wait for skb resource release.
		}

		if (pFileSrcContext->mediaSinkHook != NULL) {
			retStatus = pFileSrcContext->mediaSinkHook(pFileSrcContext->mediaSinkHookUserdata, &frame);
		}

		SAFE_MEMFREE(video_buf.output_buffer);
	}

CleanUp:

	CHK_LOG_ERR(retStatus);
	/* free resources */
	DLOGD("terminating media source");
	if (pFileSrcContext->mediaEosHook != NULL) {
		retStatus = pFileSrcContext->mediaEosHook(pFileSrcContext->mediaEosHookUserdata);
	}
	return (PVOID)(ULONG_PTR) retStatus;
}

PVOID sendAudioPackets(PVOID args)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) args;
	Frame frame;
	UINT32 frameSize;
	STATUS status;

	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);

	frame.presentationTs = 0;

	webrtc_audio_buf_t audio_buf;

	while (!ATOMIC_LOAD_BOOL(&pFileSrcContext->shutdownFileSrc)) {
		if (xQueueReceive(kvsWebrtcAudioSendQueue, &audio_buf, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		frame.flags = FRAME_FLAG_KEY_FRAME;
		frame.frameData = audio_buf.data_buf;
		frame.size = audio_buf.size;
		frame.presentationTs = getEpochTimestampInHundredsOfNanos(&audio_buf.timestamp);

		frame.trackId = DEFAULT_AUDIO_TRACK_ID;
		//frame.duration = 0;
		frame.version = FRAME_CURRENT_VERSION;
		frame.decodingTs = frame.presentationTs;

		// wait for skb resource release
		if ((skbdata_used_num > (max_skb_buf_num - 64)) || (skbbuf_used_num > (max_local_skb_num - 64))) {
			if (audio_buf.data_buf != NULL) {
				SAFE_MEMFREE(audio_buf.data_buf);
			}
			//skip this frame and wait for skb resource release.
			continue;
		}

		if (pFileSrcContext->mediaSinkHook != NULL) {
			retStatus = pFileSrcContext->mediaSinkHook(pFileSrcContext->mediaSinkHookUserdata, &frame);
		}

		SAFE_MEMFREE(audio_buf.data_buf);
	}

CleanUp:

	CHK_LOG_ERR(retStatus);
	/* free resources */
	DLOGD("terminating media source");
	if (pFileSrcContext->mediaEosHook != NULL) {
		retStatus = pFileSrcContext->mediaEosHook(pFileSrcContext->mediaEosHookUserdata);
	}
	return (PVOID)(ULONG_PTR) retStatus;
}

#ifdef ENABLE_AUDIO_SENDRECV
void sampleFrameHandler(uint64_t customData, PFrame pFrame)
{
	UNUSED_PARAM(customData);
	DLOGV("Frame received. TrackId: %" PRIu64 ", Size: %u, Flags %u", pFrame->trackId, pFrame->size, pFrame->flags);

	webrtc_audio_buf_t remote_audio;
	remote_audio.data_buf = malloc(pFrame->size);
	memcpy(remote_audio.data_buf, pFrame->frameData, pFrame->size);
	remote_audio.size = pFrame->size;
	remote_audio.timestamp = pFrame->presentationTs / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
	remote_audio.type =  AUDIO_OPUS ? AV_CODEC_ID_OPUS : (AUDIO_G711_MULAW ? AV_CODEC_ID_PCMU : AV_CODEC_ID_PCMA);

	if (xQueueSend(kvsWebrtcAudioRecvQueue, (void *)&remote_audio, NULL) != pdTRUE) {
		DLOGV("\n\rAudio_sound queue full.\n\r");
		if (remote_audio.data_buf) {
			free(remote_audio.data_buf);
		}
	}

	PStreamingSession pStreamingSession = (PStreamingSession) customData;
	if (pStreamingSession->firstFrame) {
		pStreamingSession->firstFrame = FALSE;
		pStreamingSession->startUpLatency = (GETTIME() - pStreamingSession->offerReceiveTime) / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
		DLOGD("Start up latency from offer to first frame: %" PRIu64 "ms\n", pStreamingSession->startUpLatency);
	}
}

void *sampleReceiveAudioFrame(void *args)
{
	STATUS retStatus = STATUS_SUCCESS;
	PStreamingSession pStreamingSession = (PStreamingSession) args;
	if (pStreamingSession == NULL) {
		DLOGD("[KVS Master] sampleReceiveAudioFrame(): operation returned status code: 0x%08x \n", STATUS_NULL_ARG);
		goto CleanUp;
	}

	retStatus = transceiverOnFrame(pStreamingSession->pAudioRtcRtpTransceiver, (uint64_t) pStreamingSession, sampleFrameHandler);
	if (retStatus != STATUS_SUCCESS) {
		DLOGD("[KVS Master] transceiverOnFrame(): operation returned status code: 0x%08x \n", retStatus);
		goto CleanUp;
	}

CleanUp:
	pStreamingSession->receiveAudioVideoSenderTid = INVALID_TID_VALUE;
	THREAD_EXIT(NULL);
	return (PVOID)(ULONG_PTR) retStatus;
}
#endif /* ENABLE_AUDIO_SENDRECV */

STATUS initMediaSource(PMediaContext *ppMediaContext)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = NULL;
	PCodecConfiguration pGstConfiguration;
	PCodecStreamConf pVideoStream;
	PCodecStreamConf pAudioStream;

	CHK(ppMediaContext != NULL, STATUS_MEDIA_NULL_ARG);
	*ppMediaContext = NULL;
	CHK(NULL != (pFileSrcContext = (PFileSrcContext) MEMCALLOC(1, SIZEOF(FileSrcContext))), STATUS_MEDIA_NOT_ENOUGH_MEMORY);
	ATOMIC_STORE_BOOL(&pFileSrcContext->shutdownFileSrc, FALSE);
	ATOMIC_STORE_BOOL(&pFileSrcContext->codecConfigLatched, TRUE);

	pGstConfiguration = &pFileSrcContext->codecConfiguration;
	pGstConfiguration->codecStatus = STATUS_SUCCESS;
	pVideoStream = &pGstConfiguration->videoStream;
	pAudioStream = &pGstConfiguration->audioStream;
	pVideoStream->codec = RTC_CODEC_H264_PROFILE_42E01F_LEVEL_ASYMMETRY_ALLOWED_PACKETIZATION_MODE;
#if AUDIO_OPUS
	printf("AUDIO_OPUS\r\n");
	pAudioStream->codec = RTC_CODEC_OPUS;
#elif AUDIO_G711_MULAW
	printf("AUDIO_G711_MULAW\r\n");
	pAudioStream->codec = RTC_CODEC_MULAW;
#elif AUDIO_G711_ALAW
	printf("AUDIO_G711_ALAW\r\n");
	pAudioStream->codec = RTC_CODEC_ALAW;
#endif

	pFileSrcContext->codecConfLock = MUTEX_CREATE(TRUE);
	CHK(IS_VALID_MUTEX_VALUE(pFileSrcContext->codecConfLock), STATUS_MEDIA_INVALID_MUTEX);

	// get the sdp information of rtsp server.
	//CHK_STATUS((discoverMediaSource(pFileSrcContext)));
	*ppMediaContext = pFileSrcContext;

CleanUp:

	if (STATUS_FAILED(retStatus)) {
		if (pFileSrcContext != NULL) {
			detroyMediaSource(pFileSrcContext);
		}
	}

	return retStatus;
}

STATUS isMediaSourceReady(PMediaContext pMediaContext)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) pMediaContext;
	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);
	//if (!ATOMIC_LOAD_BOOL(&pFileSrcContext->codecConfigLatched)) {
	//    discoverMediaSource(pFileSrcContext);
	//}

	if (ATOMIC_LOAD_BOOL(&pFileSrcContext->codecConfigLatched)) {
		retStatus = STATUS_SUCCESS;
	} else {
		retStatus = STATUS_MEDIA_NOT_READY;
	}

CleanUp:

	return retStatus;
}

STATUS queryMediaVideoCap(PMediaContext pMediaContext, RTC_CODEC *pCodec)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) pMediaContext;
	PCodecStreamConf pVideoStream;
	CHK((pFileSrcContext != NULL) && (pCodec != NULL), STATUS_MEDIA_NULL_ARG);
	CHK(ATOMIC_LOAD_BOOL(&pFileSrcContext->codecConfigLatched), STATUS_MEDIA_NOT_READY);
	pVideoStream = &pFileSrcContext->codecConfiguration.videoStream;
	*pCodec = pVideoStream->codec;
CleanUp:
	return retStatus;
}

STATUS queryMediaAudioCap(PMediaContext pMediaContext, RTC_CODEC *pCodec)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) pMediaContext;
	PCodecStreamConf pAudioStream;
	CHK((pFileSrcContext != NULL), STATUS_MEDIA_NULL_ARG);
	CHK(ATOMIC_LOAD_BOOL(&pFileSrcContext->codecConfigLatched), STATUS_MEDIA_NOT_READY);
	pAudioStream = &pFileSrcContext->codecConfiguration.audioStream;
	*pCodec = pAudioStream->codec;
CleanUp:
	return retStatus;
}

STATUS linkMeidaSinkHook(PMediaContext pMediaContext, MediaSinkHook mediaSinkHook, PVOID udata)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) pMediaContext;
	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);
	pFileSrcContext->mediaSinkHook = mediaSinkHook;
	pFileSrcContext->mediaSinkHookUserdata = udata;
CleanUp:
	return retStatus;
}

STATUS linkMeidaEosHook(PMediaContext pMediaContext, MediaEosHook mediaEosHook, PVOID udata)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) pMediaContext;
	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);
	pFileSrcContext->mediaEosHook = mediaEosHook;
	pFileSrcContext->mediaEosHookUserdata = udata;
CleanUp:
	return retStatus;
}

PVOID runMediaSource(PVOID args)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) args;
	TID videoSenderTid = INVALID_TID_VALUE, audioSenderTid = INVALID_TID_VALUE;

	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);

	ATOMIC_STORE_BOOL(&pFileSrcContext->shutdownFileSrc, FALSE);
	DLOGI("media source is starting");
	THREAD_CREATE_EX(&videoSenderTid, SAMPLE_VIDEO_THREAD_NAME, SAMPLE_VIDEO_THREAD_SIZE, sendVideoPackets, (PVOID) pFileSrcContext);
	THREAD_CREATE_EX(&audioSenderTid, SAMPLE_AUDIO_THREAD_NAME, SAMPLE_AUDIO_THREAD_SIZE, sendAudioPackets, (PVOID) pFileSrcContext);

	if (videoSenderTid != INVALID_TID_VALUE) {
		//#TBD, the thread_join does not work.
		THREAD_JOIN(videoSenderTid, NULL);
	}

	if (audioSenderTid != INVALID_TID_VALUE) {
		THREAD_JOIN(audioSenderTid, NULL);
	}
CleanUp:
	return (PVOID)(ULONG_PTR) retStatus;
}

#ifdef ENABLE_AUDIO_SENDRECV
PVOID runMediaReceive(PVOID args)
{
	STATUS retStatus = STATUS_SUCCESS;
	PStreamingSession pStreamingSession = (PStreamingSession) args;

	DLOGI("audio receiver is starting");
	THREAD_CREATE(&pStreamingSession->receiveAudioVideoSenderTid, sampleReceiveAudioFrame, (PVOID) pStreamingSession);
	THREAD_DETACH(pStreamingSession->receiveAudioVideoSenderTid);

CleanUp:
	return (PVOID)(ULONG_PTR) retStatus;
}
#endif /* ENABLE_AUDIO_SENDRECV */

STATUS shutdownMediaSource(PMediaContext pMediaContext)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext = (PFileSrcContext) pMediaContext;
	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);
	ATOMIC_STORE_BOOL(&pFileSrcContext->shutdownFileSrc, TRUE);
	DLOGD("shutdown media source");
CleanUp:
	return retStatus;
}

STATUS detroyMediaSource(PMediaContext *ppMediaContext)
{
	STATUS retStatus = STATUS_SUCCESS;
	PFileSrcContext pFileSrcContext;

	CHK(ppMediaContext != NULL, STATUS_MEDIA_NULL_ARG);
	pFileSrcContext = (PFileSrcContext) * ppMediaContext;
	CHK(pFileSrcContext != NULL, STATUS_MEDIA_NULL_ARG);
	if (IS_VALID_MUTEX_VALUE(pFileSrcContext->codecConfLock)) {
		MUTEX_FREE(pFileSrcContext->codecConfLock);
	}

	MEMFREE(pFileSrcContext);
	*ppMediaContext = pFileSrcContext = NULL;
CleanUp:
	return retStatus;
}

/**
 * @brief return epoch time in hundreds of nanosecond
*/
uint64_t getEpochTimestampInHundredsOfNanos(void *pTick)
{
	uint64_t timestamp;

	long sec;
	long usec;
	unsigned int tick;
	unsigned int tickDiff;

	sntp_get_lasttime(&sec, &usec, &tick);

	if ((void *)pTick == NULL) {
		tickDiff = xTaskGetTickCount() - tick;
	} else {
		tickDiff = (*(PUINT32)pTick) - tick;
	}

	sec += tickDiff / configTICK_RATE_HZ;
	usec += ((tickDiff % configTICK_RATE_HZ) / portTICK_RATE_MS) * 1000;

	if (usec >= 1000000) {
		usec -= 1000000;
		sec ++;
	}

	timestamp = ((uint64_t)sec * 1000 + usec / 1000) * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;

	return timestamp;
}
