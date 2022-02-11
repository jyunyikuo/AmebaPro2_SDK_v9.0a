
### default option ###
option(USE_DATA_CHANNEL "Use data channel" ON)

### include .cmake need if neeeded ###
include(${prj_root}/src/amazon_kvs/lib_amazon/libsrtp2.cmake)
include(${prj_root}/src/amazon_kvs/lib_amazon/libusrsctp.cmake)
include(${prj_root}/src/amazon_kvs/lib_amazon/libwslay.cmake)
include(${prj_root}/src/amazon_kvs/lib_amazon/libllhttp.cmake)
include(${prj_root}/src/amazon_kvs/lib_amazon/libkvs_webrtc.cmake)

### add linked library ###
list(
	APPEND app_example_lib
	kvs_webrtc
	srtp2
	wslay
	llhttp
	usrsctp
)

### add flags ###
list(
	APPEND app_example_flags
	KVS_PLAT_RTK_FREERTOS
	# ${kvs_webrtc_flags}
)
if(USE_DATA_CHANNEL)
list(
	APPEND app_example_flags
	ENABLE_DATA_CHANNEL
)
endif()

### add header files ###
list (
	APPEND app_example_inc_path
	"${prj_root}/src/amazon_kvs/lib_amazon/gcc_include"
	"${prj_root}/src/mmfv2_video_example"
	"${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/include"
)

### add source file ###
list(
	APPEND app_example_sources
	${sdk_root}/component/example/kvs_webrtc_mmf/app_example.c
	${sdk_root}/component/example/kvs_webrtc_mmf/module_kvs_webrtc.c
	${sdk_root}/component/example/kvs_webrtc_mmf/example_kvs_webrtc_mmf.c
	${sdk_root}/component/example/kvs_webrtc_mmf/example_kvs_webrtc_playback_mmf.c

	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppCommon.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppCredential.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppDataChannel.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppMediaSrc.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppMain.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppMessageQueue.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppMetrics.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppSignaling.c
	${sdk_root}/component/example/kvs_webrtc_mmf/webrtc_app_src/AppWebRTC.c
)
