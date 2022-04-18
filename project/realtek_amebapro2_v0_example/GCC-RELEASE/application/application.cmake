cmake_minimum_required(VERSION 3.6)

project(app)

enable_language(C CXX ASM)

if(BUILD_TZ)
set(app application.ns)
else()
set(app application.ntz)
endif()

include(../includepath.cmake)
link_directories(${prj_root}/GCC-RELEASE/application/output)

ADD_LIBRARY (bt_upperstack_lib STATIC IMPORTED )
SET_PROPERTY ( TARGET bt_upperstack_lib PROPERTY IMPORTED_LOCATION ${sdk_root}/component/bluetooth/rtk_stack/platform/amebapro2/lib/btgap.a )

if(NOT BUILD_TZ)
ADD_LIBRARY (hal_pmc_lib STATIC IMPORTED )
SET_PROPERTY ( TARGET hal_pmc_lib PROPERTY IMPORTED_LOCATION ${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/lib/hal_pmc.a )
endif()

#HAL
if(NOT BUILD_TZ)
#build TZ, move to secure project
list(
    APPEND app_sources
	
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_flash.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_flash_nsc.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_flash_sec.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_hkdf.c
	#${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_otp_nsc.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_pinmux_nsc.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_snand.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_spic.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_spic_nsc.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_wdt.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram_s/hal_rtc.c
)
endif()

list(
    APPEND app_sources
	
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_audio.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_adc.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_comp.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_crypto.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_dram_init.c	
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_dram_scan.c	
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_eddsa.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_gdma.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_gpio.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_i2c.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_i2s.c
	#${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_otp.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_pwm.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_rsa.c
	#${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_sdhost.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_ssi.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_timer.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_trng.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_uart.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_sgpio.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/rtl8735b_i2s.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/rtl8735b_sgpio.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/rtl8735b_sport.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/rtl8735b_ssi.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/rtl8735b_audio.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/hal_eth.c
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/source/ram/rtl8735b_eth.c
	
	
)

#MBED
list(
    APPEND app_sources
	${sdk_root}/component/mbed/targets/hal/rtl8735b/audio_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/crypto_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/dma_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/flash_api.c
	${sdk_root}/component/soc/8735b/misc/driver/flash_api_ext.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/i2c_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/i2s_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/pwmout_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/sgpio_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/spi_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/timer_api.c
	${sdk_root}/component/soc/8735b/mbed-drivers/source/wait_api.c
	${sdk_root}/component/soc/8735b/mbed-drivers/source/us_ticker_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/us_ticker.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/gpio_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/gpio_irq_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/serial_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/wdt_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/rtc_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/analogin_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/pinmap_common.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/pinmap.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/ethernet_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/trng_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/power_mode_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/snand_api.c
	${sdk_root}/component/mbed/targets/hal/rtl8735b/sys_api.c
)

#RTOS
list(
    APPEND app_sources
	${sdk_root}/component/os/freertos/${freertos}/Source/croutine.c
	${sdk_root}/component/os/freertos/${freertos}/Source/event_groups.c
	${sdk_root}/component/os/freertos/${freertos}/Source/list.c
	${sdk_root}/component/os/freertos/${freertos}/Source/queue.c
	${sdk_root}/component/os/freertos/${freertos}/Source/stream_buffer.c
	${sdk_root}/component/os/freertos/${freertos}/Source/tasks.c
	${sdk_root}/component/os/freertos/${freertos}/Source/timers.c
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/MemMang/heap_4_2.c
	
	${sdk_root}/component/os/freertos/freertos_cb.c
	${sdk_root}/component/os/freertos/freertos_service.c
	${sdk_root}/component/os/freertos/cmsis_os.c
	
	${sdk_root}/component/os/os_dep/osdep_service.c
	${sdk_root}/component/os/os_dep/device_lock.c
	${sdk_root}/component/os/os_dep/timer_service.c
	#posix
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_clock.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_mqueue.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_barrier.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_cond.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_mutex.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_sched.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_semaphore.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_timer.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_unistd.c
	${sdk_root}/component/os/freertos/freertos_posix/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_utils.c
)

if(BUILD_TZ)
list(
    APPEND app_sources
	#FREERTOS
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33/non_secure/port.c
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33/non_secure/portasm.c
	#CMSIS
	${sdk_root}/component/soc/8735b/cmsis/rtl8735b/source/ram_ns/app_start.c
	${sdk_root}/component/soc/8735b/cmsis/rtl8735b/source/ram_ns/system_ns.c
)
else()
list(
    APPEND app_sources
	#FREERTOS
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
	#CMSIS
	${sdk_root}/component/soc/8735b/cmsis/rtl8735b/source/ram_s/app_start.c
)
endif()

#CMSIS
list(
    APPEND app_sources
	${sdk_root}/component/soc/8735b/cmsis/rtl8735b/source/ram/mpu_config.c

	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/BasicMathFunctions/arm_add_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/TransformFunctions/arm_bitreversal2.S
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/TransformFunctions/arm_cfft_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/TransformFunctions/arm_cfft_radix8_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/ComplexMathFunctions/arm_cmplx_mag_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/CommonTables/arm_common_tables.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/CommonTables/arm_const_structs.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/StatisticsFunctions/arm_max_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/BasicMathFunctions/arm_mult_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/TransformFunctions/arm_rfft_fast_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/TransformFunctions/arm_rfft_fast_init_f32.c
	${sdk_root}/component/soc/8735b/cmsis/cmsis-dsp/source/BasicMathFunctions/arm_scale_f32.c
)

#at_cmd
list(
	APPEND app_sources
	${sdk_root}/component/at_cmd/atcmd_sys.c
	${sdk_root}/component/at_cmd/atcmd_wifi.c
	${sdk_root}/component/at_cmd/atcmd_bt.c
	${sdk_root}/component/at_cmd/atcmd_mp.c
	${sdk_root}/component/at_cmd/atcmd_mp_ext2.c
	${sdk_root}/component/at_cmd/atcmd_isp.c
	${sdk_root}/component/at_cmd/atcmd_ftl.c
	${sdk_root}/component/at_cmd/log_service.c
	${sdk_root}/component/soc/8735b/misc/driver/rtl_console.c
	${sdk_root}/component/soc/8735b/misc/driver/low_level_io.c
)

#wifi
list(
	APPEND app_sources
	#api
	${sdk_root}/component/wifi/api/wifi_conf.c
	${sdk_root}/component/wifi/api/wifi_conf_wowlan.c
	${sdk_root}/component/wifi/api/wifi_conf_inter.c
	${sdk_root}/component/wifi/api/wifi_conf_ext.c
	${sdk_root}/component/wifi/api/wifi_ind.c
	${sdk_root}/component/wifi/api/wlan_network.c
	#promisc
	${sdk_root}/component/wifi/promisc/wifi_conf_promisc.c
	${sdk_root}/component/wifi/promisc/wifi_promisc.c
	#fast_connect
	${sdk_root}/component/wifi/wifi_fast_connect/wifi_fast_connect.c
	#wpa_supplicant
	${sdk_root}/component/wifi/wpa_supplicant/wpa_supplicant/wifi_wps_config.c
	#option
	${sdk_root}/component/wifi/driver/src/core/option/rtw_opt_crypto_ssl.c
	${sdk_root}/component/wifi/driver/src/core/option/rtw_opt_skbuf_rtl8735b.c
)

#network
list(
	APPEND app_sources
	#dhcp
	${sdk_root}/component/network/dhcp/dhcps.c
	#ping
	${sdk_root}/component/network/ping/ping_test.c
	#iperf
	${sdk_root}/component/network/iperf/iperf.c
	#sntp
	${sdk_root}/component/network/sntp/sntp.c
	#http
	${sdk_root}/component/network/httpc/httpc_tls.c
	${sdk_root}/component/network/httpd/httpd_tls.c
	#cJSON
	${sdk_root}/component/network/cJSON/cJSON.c
	#ssl_client
	${sdk_root}/component/example/ssl_client/ssl_client.c
	${sdk_root}/component/example/ssl_client/ssl_client_ext.c
	#mqtt
	${sdk_root}/component/network/mqtt/MQTTClient/MQTTClient.c
	${sdk_root}/component/network/mqtt/MQTTClient/MQTTFreertos.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTConnectClient.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTConnectServer.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTDeserializePublish.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTFormat.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTPacket.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTSerializePublish.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTSubscribeClient.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTSubscribeServer.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTUnsubscribeClient.c
	${sdk_root}/component/network/mqtt/MQTTPacket/MQTTUnsubscribeServer.c
	#ota
	${sdk_root}/component/soc/8735b/misc/platform/ota_8735b.c
	#httplite
	${sdk_root}/component/network/httplite/http_client.c
	#xml
	${sdk_root}/component/network/xml/xml.c
)

#lwip
list(
	APPEND app_sources
	#api
	${sdk_root}/component/lwip/api/lwip_netconf.c
	#lwip - api
	${sdk_root}/component/lwip/${lwip}/src/api/api_lib.c
	${sdk_root}/component/lwip/${lwip}/src/api/api_msg.c
	${sdk_root}/component/lwip/${lwip}/src/api/err.c
	${sdk_root}/component/lwip/${lwip}/src/api/netbuf.c
	${sdk_root}/component/lwip/${lwip}/src/api/netdb.c
	${sdk_root}/component/lwip/${lwip}/src/api/netifapi.c
	${sdk_root}/component/lwip/${lwip}/src/api/sockets.c
	${sdk_root}/component/lwip/${lwip}/src/api/tcpip.c
	#lwip - core
	${sdk_root}/component/lwip/${lwip}/src/core/def.c
	${sdk_root}/component/lwip/${lwip}/src/core/dns.c
	${sdk_root}/component/lwip/${lwip}/src/core/inet_chksum.c
	${sdk_root}/component/lwip/${lwip}/src/core/init.c
	${sdk_root}/component/lwip/${lwip}/src/core/ip.c
	${sdk_root}/component/lwip/${lwip}/src/core/mem.c
	${sdk_root}/component/lwip/${lwip}/src/core/memp.c
	${sdk_root}/component/lwip/${lwip}/src/core/netif.c
	${sdk_root}/component/lwip/${lwip}/src/core/pbuf.c
	${sdk_root}/component/lwip/${lwip}/src/core/raw.c
	${sdk_root}/component/lwip/${lwip}/src/core/stats.c
	${sdk_root}/component/lwip/${lwip}/src/core/sys.c
	${sdk_root}/component/lwip/${lwip}/src/core/tcp.c
	${sdk_root}/component/lwip/${lwip}/src/core/tcp_in.c
	${sdk_root}/component/lwip/${lwip}/src/core/tcp_out.c
	${sdk_root}/component/lwip/${lwip}/src/core/timeouts.c
	${sdk_root}/component/lwip/${lwip}/src/core/udp.c
	#lwip - core - ipv4
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/autoip.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/dhcp.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/etharp.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/icmp.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/igmp.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/ip4.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/ip4_addr.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv4/ip4_frag.c
	#lwip - core - ipv6
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/dhcp6.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/ethip6.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/icmp6.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/inet6.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/ip6.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/ip6_addr.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/ip6_frag.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/mld6.c
	${sdk_root}/component/lwip/${lwip}/src/core/ipv6/nd6.c
	#lwip - netif
	${sdk_root}/component/lwip/${lwip}/src/netif/ethernet.c
	#lwip - port
	${sdk_root}/component/lwip/${lwip}/port/realtek/freertos/ethernetif.c
	${sdk_root}/component/wifi/driver/src/osdep/lwip_intf.c
	${sdk_root}/component/lwip/${lwip}/port/realtek/freertos/sys_arch.c
)

#ssl
if(${mbedtls} STREQUAL "mbedtls-2.4.0")
list(
	APPEND app_sources
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/aesni.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/blowfish.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/camellia.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ccm.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/certs.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/cipher.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/cipher_wrap.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/cmac.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/debug.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/gcm.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/havege.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/md2.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/md4.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/memory_buffer_alloc.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/net_sockets.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/padlock.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/pkcs11.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/pkcs12.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/pkcs5.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/pkparse.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/platform.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ripemd160.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ssl_cache.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ssl_ciphersuites.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ssl_cli.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ssl_cookie.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ssl_srv.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ssl_ticket.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/ssl_tls.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/threading.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/timing.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/version.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/version_features.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/x509.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/x509_create.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/x509_crl.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/x509_crt.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/x509_csr.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/x509write_crt.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/x509write_csr.c
	${sdk_root}/component/ssl/mbedtls-2.4.0/library/xtea.c
	#ssl_ram_map
	${sdk_root}/component/ssl/ssl_ram_map/rom/rom_ssl_ram_map.c
	${sdk_root}/component/ssl/ssl_func_stubs/ssl_func_stubs.c
)
elseif(${mbedtls} STREQUAL "mbedtls-3.0.0")
list(
	APPEND app_sources
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/aes.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/aes_alt.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/aesni.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/aria.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/asn1parse.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/asn1write.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/base64.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/bignum.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/camellia.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ccm.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/certs.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/chacha20.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/chachapoly.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/cipher.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/cipher_wrap.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/cmac.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ctr_drbg.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/debug.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/des.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/dhm.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ecdh.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ecdsa.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ecjpake.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ecp.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ecp_curves.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/entropy.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/entropy_alt.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/entropy_poll.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/error.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/gcm.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/hkdf.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/hmac_drbg.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/md.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/md5.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/memory_buffer_alloc.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/net_sockets.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/nist_kw.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/oid.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/padlock.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/pem.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/pk.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/pk_wrap.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/pkcs12.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/pkcs5.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/pkparse.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/pkwrite.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/platform.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/platform_util.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/poly1305.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ripemd160.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/rsa.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/rsa_alt_helpers.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/sha1.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/sha256.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/sha512.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_cache.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_ciphersuites.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_cli.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_cookie.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_msg.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_srv.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_ticket.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/ssl_tls.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/threading.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/timing.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/version.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/version_features.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/x509.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/x509_create.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/x509_crl.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/x509_crt.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/x509_csr.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/x509write_crt.c
	${sdk_root}/component/ssl/mbedtls-3.0.0/library/x509write_csr.c
	#ssl_ram_map
	${sdk_root}/component/ssl/ssl_ram_map/rom/rom_ssl_ram_map.c
	${sdk_root}/component/ssl/ssl_func_stubs/ssl_func_stubs.c
)
elseif(${mbedtls} STREQUAL "mbedtls-2.16.6")
file(GLOB MBEDTLS_SRC CONFIGURE_DEPENDS ${sdk_root}/component/ssl/mbedtls-2.16.6/library/*.c)
list(
	APPEND app_sources
	${MBEDTLS_SRC}
	#ssl_ram_map
	${sdk_root}/component/ssl/ssl_ram_map/rom/rom_ssl_ram_map.c
	${sdk_root}/component/ssl/ssl_func_stubs/ssl_func_stubs.c
)
endif()

#FATFS
list(
	APPEND app_sources
	${sdk_root}/component/file_system/fatfs/disk_if/src/sdcard.c
	${sdk_root}/component/file_system/fatfs/disk_if/src/flash_fatfs.c
	${sdk_root}/component/file_system/fatfs/fatfs_ext/src/ff_driver.c
	
	${sdk_root}/component/file_system/fatfs/r0.14/diskio.c
	${sdk_root}/component/file_system/fatfs/r0.14/ff.c
	${sdk_root}/component/file_system/fatfs/r0.14/ffsystem.c
	${sdk_root}/component/file_system/fatfs/r0.14/ffunicode.c
	${sdk_root}/component/file_system/fatfs/fatfs_flash_api.c
	${sdk_root}/component/file_system/fatfs/fatfs_reent.c
	${sdk_root}/component/file_system/fatfs/fatfs_sdcard_api.c
	${sdk_root}/component/file_system/fatfs/fatfs_ramdisk_api.c
)

#Littlefs
list(
	APPEND app_sources
	${sdk_root}/component/file_system/littlefs/r2.41/lfs.c
	${sdk_root}/component/file_system/littlefs/r2.41/lfs_util.c
	${sdk_root}/component/file_system/littlefs/lfs_nor_flash_api.c
	${sdk_root}/component/file_system/littlefs/lfs_nand_flash_api.c
)

#vfs
list(
	APPEND app_sources
	${sdk_root}/component/file_system/vfs/vfs.c
	${sdk_root}/component/file_system/vfs/vfs_fatfs.c
	${sdk_root}/component/file_system/vfs/vfs_littlefs.c
)

#FTL_COMMON
list(
	APPEND app_sources
	${sdk_root}/component/file_system/ftl_common/ftl_common_api.c
	${sdk_root}/component/file_system/ftl_common/ftl_nand_api.c
	${sdk_root}/component/file_system/ftl_common/ftl_nor_api.c
	#${sdk_root}/component/file_system/ftl_common/nand_task.c
)

#system_data
list(
	APPEND app_sources
	${sdk_root}/component/file_system/system_data/system_data_api.c
)

#USER
list(
    APPEND app_sources
	${prj_root}/src/main.c
)

#rtw_wpa_supplicant
list(
	APPEND app_sources
	${sdk_root}/component/wifi/wifi_config/wifi_simple_config.c
)

#RTSP
list(
	APPEND app_sources	
	${sdk_root}/component/network/rtsp/rtp_api.c
	${sdk_root}/component/network/rtsp/rtsp_api.c
	${sdk_root}/component/network/rtsp/sdp.c
)

#MMF_MODULE
list(
	APPEND app_sources	
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/sensor_gc2053.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/sensor_ps5258.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/iq_gc2053_bin.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/iq_ps5258_bin.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/sensor_gc4653.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/iq_gc4653_bin.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/sensor_mis2008.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/iq_mis2008_bin.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/sensor_imx307.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/iq_imx307_bin.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/sensor_imx307hdr.s
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/iq_imx307hdr_bin.s
	#${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin/voe_bin.s
	${sdk_root}/component/media/mmfv2/module_video.c
	${sdk_root}/component/media/mmfv2/module_rtsp2.c
	${sdk_root}/component/media/mmfv2/module_array.c
	${sdk_root}/component/media/mmfv2/module_audio.c
	${sdk_root}/component/media/mmfv2/module_aac.c
	${sdk_root}/component/media/mmfv2/module_aad.c
	${sdk_root}/component/media/mmfv2/module_g711.c
	#${sdk_root}/component/media/mmfv2/module_httpfs.c
	${sdk_root}/component/media/mmfv2/module_i2s.c
	${sdk_root}/component/media/mmfv2/module_mp4.c
	${sdk_root}/component/media/mmfv2/module_rtp.c
    ${sdk_root}/component/media/mmfv2/module_opusc.c
	${sdk_root}/component/media/mmfv2/module_opusd.c
    ${sdk_root}/component/media/mmfv2/module_uvcd.c
    ${sdk_root}/component/media/mmfv2/module_demuxer.c
    ${sdk_root}/component/media/mmfv2/module_md.c
    ${sdk_root}/component/media/mmfv2/module_fileloader.c
    ${sdk_root}/component/media/mmfv2/module_filesaver.c
)

#MISC
list(
	APPEND app_sources
	
	${sdk_root}/component/soc/8735b/misc/utilities/source/ram/libc_wrap.c
	${sdk_root}/component/soc/8735b/app/shell/cmd_shell.c	
)

#LIB
list(
	APPEND app_sources
	
)

#FTL
list(
	APPEND app_sources
	
	${sdk_root}/component/file_system/ftl/ftl.c
)

#FWFS
list(
	APPEND app_sources
	
	${sdk_root}/component/file_system/fwfs/fwfs.c
)


#BLUETOOTH
list(
	APPEND app_sources

	${sdk_root}/component/bluetooth/driver/hci/hci_process/hci_process.c
	${sdk_root}/component/bluetooth/driver/hci/hci_process/hci_standalone.c
	${sdk_root}/component/bluetooth/driver/hci/hci_transport/hci_h4.c
	${sdk_root}/component/bluetooth/driver/hci/hci_if_rtk.c
	${sdk_root}/component/bluetooth/driver/platform/amebapro2/hci/bt_mp_patch.c
	${sdk_root}/component/bluetooth/driver/platform/amebapro2/hci/bt_normal_patch.c
	${sdk_root}/component/bluetooth/driver/platform/amebapro2/hci/hci_dbg.c
	${sdk_root}/component/bluetooth/driver/platform/amebapro2/hci/hci_platform.c
	${sdk_root}/component/bluetooth/driver/platform/amebapro2/hci/hci_uart.c
	${sdk_root}/component/bluetooth/rtk_stack/platform/amebapro2/src/platform_utils.c
	${sdk_root}/component/bluetooth/rtk_stack/platform/amebapro2/src/rtk_coex.c
	${sdk_root}/component/bluetooth/rtk_stack/platform/amebapro2/src/trace_uart.c
	${sdk_root}/component/bluetooth/rtk_stack/platform/amebapro2/src/uart_bridge.c
	${sdk_root}/component/bluetooth/rtk_stack/platform/common/src/cycle_queue.c
	${sdk_root}/component/bluetooth/rtk_stack/platform/common/src/trace_task.c
	${sdk_root}/component/bluetooth/os/freertos/osif_freertos.c

	${sdk_root}/component/bluetooth/rtk_stack/src/ble/profile/server/simple_ble_service.c
	${sdk_root}/component/bluetooth/rtk_stack/src/ble/profile/server/bas.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_peripheral/app_task.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_peripheral/ble_app_main.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_peripheral/peripheral_app.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_peripheral/ble_peripheral_at_cmd.c

	${sdk_root}/component/bluetooth/rtk_stack/src/ble/profile/client/gcs_client.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_central/ble_central_app_main.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_central/ble_central_app_task.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_central/ble_central_client_app.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_central/ble_central_link_mgr.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_central/ble_central_at_cmd.c

	${sdk_root}/component/bluetooth/rtk_stack/example/ble_scatternet/ble_scatternet_app_main.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_scatternet/ble_scatternet_app_task.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_scatternet/ble_scatternet_app.c
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_scatternet/ble_scatternet_link_mgr.c

	${sdk_root}/component/bluetooth/rtk_stack/example/bt_beacon/bt_beacon_app.c
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_beacon/bt_beacon_app_main.c
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_beacon/bt_beacon_app_task.c

	${sdk_root}/component/bluetooth/rtk_stack/example/bt_config/bt_config_app_main.c
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_config/bt_config_app_task.c
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_config/bt_config_peripheral_app.c
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_config/bt_config_service.c
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_config/bt_config_wifi.c
)

#NN MODEL
list(
	APPEND app_sources
	${prj_root}/src/test_model/model_mbnetssd.c
	${prj_root}/src/test_model/model_yolov3t.c
    ${prj_root}/src/test_model/model_yamnet.c
    ${prj_root}/src/test_model/model_yamnet_s.c
	${prj_root}/src/test_model/mel_spectrogram.c
	${prj_root}/src/test_model/wave_sample/DCBOS2UWKAA_30_0.c
	${prj_root}/src/test_model/wave_sample/Pgprrf93CtE_30_2.c
	${prj_root}/src/test_model/wave_sample/KOS5gxwxFlI_170_0.c
	${prj_root}/src/test_model/wave_sample/YpGd1FUqzwY_0_0.c
	${prj_root}/src/test_model/wave_sample/ZthT5nwFkJg_60_1.c
	${prj_root}/src/test_model/wave_sample/fvYPQygklIo_30_2.c
	${prj_root}/src/test_model/wave_sample/jSC3k_UgPOI_120_2.c
	${prj_root}/src/test_model/wave_sample/vUXCWzZyMew_20_1.c
)
#NN example
list(
	APPEND app_sources

	${sdk_root}/component/media/mmfv2/module_vipnn.c
	
	${sdk_root}/component/media/mmfv2/module_facerecog.c
	${sdk_root}/component/media/mmfv2/module_mbnssd.c
)

if(PICOLIBC)
list(
	APPEND app_sources

	${sdk_root}/component/soc/8735b/misc/driver/picolibc/getentropy.c
)
endif()

if(DEFINED EXAMPLE AND EXAMPLE)
    message(STATUS "EXAMPLE = ${EXAMPLE}")
    if(EXISTS ${sdk_root}/component/example/${EXAMPLE})
		if(EXISTS ${sdk_root}/component/example/${EXAMPLE}/${EXAMPLE}.cmake)
			message(STATUS "Found ${EXAMPLE} include project")
			include(${sdk_root}/component/example/${EXAMPLE}/${EXAMPLE}.cmake)
		else()
			message(WARNING "Found ${EXAMPLE} include project but ${EXAMPLE}.cmake not exist")
		endif()
    else()
        message(WARNING "${EXAMPLE} Not Found")
    endif()
    if(NOT DEBUG)
        set(EXAMPLE OFF CACHE STRING INTERNAL FORCE)
    endif()
elseif(DEFINED VIDEO_EXAMPLE AND VIDEO_EXAMPLE)
    message(STATUS "Build VIDEO_EXAMPLE project")
    include(${prj_root}/src/mmfv2_video_example/video_example_media_framework.cmake)
    if(NOT DEBUG)
        set(VIDEO_EXAMPLE OFF CACHE STRING INTERNAL FORCE)
    endif()
elseif(DEFINED DOORBELL_CHIME AND DOORBELL_CHIME)
    message(STATUS "Build DOORBELL_CHIME project")
    include(${prj_root}/src/doorbell-chime/doorbell-chime.cmake)
    if(NOT DEBUG)
        set(DOORBELL_CHIME OFF CACHE STRING INTERNAL FORCE)
    endif()
else()
endif()

if(BUILD_TZ)
	add_library(secure_object OBJECT IMPORTED)
	set_target_properties( secure_object PROPERTIES IMPORTED_OBJECTS "${CMAKE_CURRENT_BINARY_DIR}/import_lib.o")

	add_executable(
		${app}
		${app_sources}
		${app_example_sources}
		$<TARGET_OBJECTS:secure_object>
	)

	# add noddr ld??
	set( soclib soc_ns)
	set( ld_script ${CMAKE_CURRENT_SOURCE_DIR}/rtl8735b_ram_ns.ld )
else()
	add_executable(
		${app}
		${app_sources}
		${app_example_sources}
	)

	set( soclib soc_ntz)
	if(NODDR)
		message(STATUS "WITHOUT DDR")
		set( ld_script ${CMAKE_CURRENT_SOURCE_DIR}/rtl8735b_ram_noddr.ld ) 
	else()
		message(STATUS "WITH DDR")
		set( ld_script ${CMAKE_CURRENT_SOURCE_DIR}/rtl8735b_ram.ld )
	endif()	
endif()


list(
	APPEND app_flags
	${app_example_flags}
	CONFIG_BUILD_RAM=1 
	CONFIG_BUILD_LIB=1 
	CONFIG_PLATFORM_8735B
	CONFIG_RTL8735B_PLATFORM=1
	CONFIG_SYSTEM_TIME64=1
)

if(BUILD_TZ)
list(
	APPEND app_flags
	CONFIG_BUILD_NONSECURE=1
	ENABLE_SECCALL_PATCH
)
endif()

target_compile_definitions(${app} PRIVATE ${app_flags} )

# HEADER FILE PATH
target_include_directories(
	${app}
	PUBLIC

	${inc_path}
	${app_example_inc_path}
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33/non_secure
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33/secure
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/voe_bin
	${sdk_root}/component/video/driver/RTL8735B
	
	${prj_root}/src/test_model
	${prj_root}/src
	
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/nn
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/nn/model_itp
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/nn/nn_api
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/nn/nn_postprocess
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/nn/nn_preprocess
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/nn/run_facerecog
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/nn/run_itp	
	
	${sdk_root}/component/soc/8735b/misc/platform

	${sdk_root}/component/media/mmfv2
	${sdk_root}/component/media/rtp_codec
	${sdk_root}/component/audio/3rdparty/AEC
	${sdk_root}/component/mbed/hal_ext
	${sdk_root}/component/file_system/ftl
	${sdk_root}/component/file_system/system_data
	${sdk_root}/component/file_system/fwfs

	${sdk_root}/component/bluetooth/driver
	${sdk_root}/component/bluetooth/driver/hci
	${sdk_root}/component/bluetooth/driver/inc
	${sdk_root}/component/bluetooth/driver/inc/hci
	${sdk_root}/component/bluetooth/driver/platform/amebapro2/inc
	${sdk_root}/component/bluetooth/os/osif
	${sdk_root}/component/bluetooth/rtk_stack/example
	${sdk_root}/component/bluetooth/rtk_stack/inc/app
	${sdk_root}/component/bluetooth/rtk_stack/inc/bluetooth/gap
	${sdk_root}/component/bluetooth/rtk_stack/inc/bluetooth/profile
	${sdk_root}/component/bluetooth/rtk_stack/inc/bluetooth/profile/client
	${sdk_root}/component/bluetooth/rtk_stack/inc/bluetooth/profile/server
	${sdk_root}/component/bluetooth/rtk_stack/inc/framework/bt
	${sdk_root}/component/bluetooth/rtk_stack/inc/framework/remote
	${sdk_root}/component/bluetooth/rtk_stack/inc/framework/sys
	${sdk_root}/component/bluetooth/rtk_stack/inc/os
	${sdk_root}/component/bluetooth/rtk_stack/inc/platform
	${sdk_root}/component/bluetooth/rtk_stack/inc/stack
	${sdk_root}/component/bluetooth/rtk_stack/platform/amebapro2/inc
	${sdk_root}/component/bluetooth/rtk_stack/platform/amebapro2/lib
	${sdk_root}/component/bluetooth/rtk_stack/platform/common/inc
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_central
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_peripheral
	${sdk_root}/component/bluetooth/rtk_stack/example/ble_scatternet
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_beacon
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_config
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_airsync_config
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_mesh/provisioner
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_mesh/device
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_mesh_multiple_profile/provisioner_multiple_profile
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_mesh_multiple_profile/device_multiple_profile
	${sdk_root}/component/bluetooth/rtk_stack/example/bt_mesh_test

	${sdk_root}/component/wifi/wpa_supplicant/src
	${sdk_root}/component/network/mqtt/MQTTClient
	${sdk_root}/component/network/mqtt/MQTTPacket
	${prj_root}/src/VIPLiteDrv/sdk/inc
	
	${sdk_root}/component/example/media_framework/inc
	${prj_root}/src/doorbell-chime
	${sdk_root}/component/wifi/wpa_supplicant/src
	${sdk_root}/component/wifi/driver/src/core/option
	${sdk_root}/component/ssl/ssl_ram_map/rom
	${sdk_root}/component/audio/3rdparty/faac/libfaac
	${prj_root}/src/VIPLiteDrv/sdk/inc
	${prj_root}/component/file_system/fatfs/r0.14
	${sdk_root}/component/soc/8735b/fwlib/rtl8735b/lib/source/ram/video/osd
	${sdk_root}/component/wifi/wifi_config
)

if(NOT BUILD_WLANMP)	
	set( wlanlib wlan)
else()
	set( wlanlib wlan_mp)
endif()

if(NOT BUILD_TZ)
target_link_libraries(
	${app}
	hal_pmc_lib
)
endif()

target_link_libraries(
	${app}
	${wlanlib}
	${app_example_lib}
	wps
	opusenc
	opusfile
	opus
	hmp3
	g711
	http
	aec
	video
	mmf
	bt_upperstack_lib
	sdcard
	faac
	haac
	muxer
	usbd
	qrcode
	nn
	${soclib}
	stdc++
	m
	c
	gcc
)



if(NOT PICOLIBC)
target_link_libraries(
	${app} 
	nosys
)
endif()

target_link_options(
	${app} 
	PUBLIC
	"LINKER:SHELL:-L ${CMAKE_CURRENT_SOURCE_DIR}/../ROM/GCC"
	"LINKER:SHELL:-L ${CMAKE_CURRENT_BINARY_DIR}"
	"LINKER:SHELL:-T ${ld_script}"
	"LINKER:SHELL:-Map=${app}.map"
	#"SHELL:${CMAKE_CURRENT_SOURCE_DIR}/build/import.lib"
)

if(BUILD_TZ)
target_link_options(
	${app} 
	PUBLIC
	"LINKER:SHELL:-wrap,hal_crypto_engine_init_platform"
	"LINKER:SHELL:-wrap,hal_pinmux_register"
	"LINKER:SHELL:-wrap,hal_pinmux_unregister"
	"LINKER:SHELL:-wrap,hal_otp_byte_rd_syss"
	"LINKER:SHELL:-wrap,hal_otp_byte_wr_syss"
	"LINKER:SHELL:-wrap,hal_sys_get_video_info"
	"LINKER:SHELL:-wrap,hal_sys_peripheral_en"
	"LINKER:SHELL:-wrap,hal_sys_set_clk"
	"LINKER:SHELL:-wrap,hal_sys_get_clk"
	"LINKER:SHELL:-wrap,hal_sys_lxbus_shared_en"
	"LINKER:SHELL:-wrap,bt_power_on"
)
endif()

set_target_properties(${app} PROPERTIES LINK_DEPENDS ${ld_script})


add_custom_command(TARGET ${app} POST_BUILD 
	COMMAND ${CMAKE_NM} $<TARGET_FILE:${app}> | sort > ${app}.nm.map
	COMMAND ${CMAKE_OBJEDUMP} -d $<TARGET_FILE:${app}> > ${app}.asm
	COMMAND cp $<TARGET_FILE:${app}> ${app}.axf
	COMMAND ${CMAKE_OBJCOPY} -j .bluetooth_trace.text -Obinary ${app}.axf APP.trace
	COMMAND ${CMAKE_OBJCOPY} -R .bluetooth_trace.text ${app}.axf 

	#COMMAND [ -d output ] || mkdir output
	COMMAND rm -rf output && mkdir output
	COMMAND cp -f ${app}.nm.map output
	COMMAND cp -f ${app}.asm output
	COMMAND cp -f ${app}.map output
	COMMAND cp -f ${app}.axf output
	COMMAND cp -f APP.trace output
	
	#COMMAND cp -f *.a output
)
