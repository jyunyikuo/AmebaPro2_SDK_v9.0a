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
#define LOG_CLASS "AppSignaling"
#include "AppSignaling.h"
#include "kvs/common_defs.h"

STATUS initAppSignaling(PAppSignaling pAppSignaling, SignalingClientMessageReceivedFunc onMessageReceived,
						SignalingClientStateChangedFunc onStateChanged, SignalingClientErrorReportFunc pOnError, UINT64 udata, BOOL useTurn)
{
	STATUS retStatus = STATUS_SUCCESS;
	pAppSignaling->signalingClientHandle = INVALID_SIGNALING_CLIENT_HANDLE_VALUE;
	pAppSignaling->useTurn = useTurn;
	pAppSignaling->signalingSendMessageLock = MUTEX_CREATE(FALSE);
	CHK(IS_VALID_MUTEX_VALUE(pAppSignaling->signalingSendMessageLock), STATUS_APP_SIGNALING_INVALID_MUTEX);
	pAppSignaling->signalingClientCallbacks.version = SIGNALING_CLIENT_CALLBACKS_CURRENT_VERSION;
	pAppSignaling->signalingClientCallbacks.messageReceivedFn = onMessageReceived;
	pAppSignaling->signalingClientCallbacks.stateChangeFn = onStateChanged;
	pAppSignaling->signalingClientCallbacks.errorReportFn = pOnError;
	pAppSignaling->signalingClientCallbacks.customData = (UINT64) udata;

CleanUp:
	return retStatus;
}

SIGNALING_CHANNEL_ROLE_TYPE getAppSignalingRole(PAppSignaling pAppSignaling)
{
	return pAppSignaling->channelInfo.channelRoleType;
}

STATUS queryAppSignalingServer(PAppSignaling pAppSignaling, PRtcIceServer pIceServer, PUINT32 pServerNum)
{
	STATUS retStatus = STATUS_SUCCESS;
	UINT32 i, j, iceConfigCount, uriCount = 0, maxTurnServer = 1;
	PIceConfigInfo pIceConfigInfo;
	*pServerNum = 0;

	// Set the  STUN server
	SNPRINTF(pIceServer[0].urls, MAX_ICE_CONFIG_URI_LEN, KINESIS_VIDEO_STUN_URL, pAppSignaling->channelInfo.pRegion);
	*pServerNum = 1;

	if (pAppSignaling->useTurn) {
		// Set the URIs from the configuration
		CHK(signalingClientGetIceConfigInfoCount(pAppSignaling->signalingClientHandle, &iceConfigCount) == STATUS_SUCCESS,
			STATUS_APP_SIGNALING_INVALID_INFO_COUNT);

		// signalingClientGetIceConfigInfoCount can return more than one turn server. Use only one to optimize
		// candidate gathering latency. But user can also choose to use more than 1 turn server.
		for (uriCount = 0, i = 0; i < maxTurnServer; i++) {
			CHK(signalingClientGetIceConfigInfo(pAppSignaling->signalingClientHandle, i, &pIceConfigInfo) == STATUS_SUCCESS,
				STATUS_APP_SIGNALING_INVALID_INFO);

			for (j = 0; j < pIceConfigInfo->uriCount; j++) {
				/*
				 * if pIceServer[uriCount + 1].urls is "turn:ip:port?transport=udp" then ICE will try TURN over UDP
				 * if pIceServer[uriCount + 1].urls is "turn:ip:port?transport=tcp" then ICE will try TURN over TCP/TLS
				 * if pIceServer[uriCount + 1].urls is "turns:ip:port?transport=udp", it's currently ignored because sdk dont do TURN
				 * over DTLS yet. if pIceServer[uriCount + 1].urls is "turns:ip:port?transport=tcp" then ICE will try TURN over TCP/TLS
				 * if pIceServer[uriCount + 1].urls is "turn:ip:port" then ICE will try both TURN over UPD and TCP/TLS
				 *
				 * It's recommended to not pass too many TURN iceServers to configuration because it will slow down ice gathering in non-trickle mode.
				 */

				STRNCPY(pIceServer[uriCount + 1].urls, pIceConfigInfo->uris[j], MAX_ICE_CONFIG_URI_LEN);
				STRNCPY(pIceServer[uriCount + 1].credential, pIceConfigInfo->password, MAX_ICE_CONFIG_CREDENTIAL_LEN);
				STRNCPY(pIceServer[uriCount + 1].username, pIceConfigInfo->userName, MAX_ICE_CONFIG_USER_NAME_LEN);
				uriCount++;
			}
		}
	}
	// add one for stun server.
	*pServerNum = uriCount + 1;
CleanUp:

	return retStatus;
}

STATUS connectAppSignaling(PAppSignaling pAppSignaling)
{
	STATUS retStatus = STATUS_SUCCESS;
	CHK(signalingClientCreate(&pAppSignaling->clientInfo, &pAppSignaling->channelInfo, &pAppSignaling->signalingClientCallbacks,
							  pAppSignaling->pAppCredential->pCredentialProvider, &pAppSignaling->signalingClientHandle) == STATUS_SUCCESS,
		STATUS_APP_SIGNALING_CREATE);
	DLOGD("Signaling client created successfully\n");
	// Enable the processing of the messages
	CHK(signalingClientConnect(pAppSignaling->signalingClientHandle) == STATUS_SUCCESS, STATUS_APP_SIGNALING_CONNECT);

CleanUp:
	return retStatus;
}

STATUS checkAppSignaling(PAppSignaling pAppSignaling)
{
	STATUS retStatus = STATUS_APP_SIGNALING_INVALID_HANDLE;
	SIGNALING_CLIENT_STATE signalingClientState;

	// Check the signaling client state and connect if needed
	if (IS_VALID_SIGNALING_CLIENT_HANDLE(pAppSignaling->signalingClientHandle)) {
		CHK(signalingClientGetCurrentState(pAppSignaling->signalingClientHandle, &signalingClientState) == STATUS_SUCCESS,
			STATUS_APP_SIGNALING_NOT_READY);
		retStatus = STATUS_SUCCESS;
		if (signalingClientState == SIGNALING_CLIENT_STATE_READY) {
			CHK(signalingClientConnect(pAppSignaling->signalingClientHandle) == STATUS_SUCCESS, STATUS_APP_SIGNALING_CONNECT);
		}
	}

CleanUp:

	return retStatus;
}

STATUS sendAppSignalingMessage(PAppSignaling pAppSignaling, PSignalingMessage pMessage)
{
	STATUS retStatus = STATUS_SUCCESS;
	BOOL locked = FALSE;

	// Validate the input params
	CHK((pAppSignaling != NULL) && (pMessage != NULL), STATUS_APP_SIGNALING_NULL_ARG);
	CHK(IS_VALID_MUTEX_VALUE(pAppSignaling->signalingSendMessageLock), STATUS_APP_SIGNALING_INVALID_MUTEX);
	CHK(IS_VALID_SIGNALING_CLIENT_HANDLE(pAppSignaling->signalingClientHandle), STATUS_APP_SIGNALING_INVALID_HANDLE);

	MUTEX_LOCK(pAppSignaling->signalingSendMessageLock);
	locked = TRUE;
	CHK(signalingClientSendMessage(pAppSignaling->signalingClientHandle, pMessage) == STATUS_SUCCESS, STATUS_APP_SIGNALING_SEND);

CleanUp:

	if (locked) {
		MUTEX_UNLOCK(pAppSignaling->signalingSendMessageLock);
	}

	CHK_LOG_ERR((retStatus));
	return retStatus;
}

STATUS restartAppSignaling(PAppSignaling pAppSignaling)
{
	STATUS retStatus = STATUS_SUCCESS;
	// Check if we need to re-create the signaling client on-the-fly
	CHK(signalingClientFree(&pAppSignaling->signalingClientHandle) == STATUS_SUCCESS, STATUS_APP_SIGNALING_RESTART);
	CHK(signalingClientCreate(&pAppSignaling->clientInfo, &pAppSignaling->channelInfo, &pAppSignaling->signalingClientCallbacks,
							  pAppSignaling->pAppCredential->pCredentialProvider, &pAppSignaling->signalingClientHandle) == STATUS_SUCCESS,
		STATUS_APP_SIGNALING_RESTART);

CleanUp:

	return retStatus;
}

STATUS freeAppSignaling(PAppSignaling pAppSignaling)
{
	STATUS retStatus = STATUS_SUCCESS;

	if (pAppSignaling->signalingClientHandle != INVALID_SIGNALING_CLIENT_HANDLE_VALUE) {
		retStatus = signalingClientFree(&pAppSignaling->signalingClientHandle);
		if (retStatus != STATUS_SUCCESS) {
			retStatus = STATUS_APP_SIGNALING_FREE;
		}
	}

	if (IS_VALID_MUTEX_VALUE(pAppSignaling->signalingSendMessageLock)) {
		MUTEX_FREE(pAppSignaling->signalingSendMessageLock);
	}

	return retStatus;
}
