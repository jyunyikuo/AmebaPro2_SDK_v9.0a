cmake_minimum_required(VERSION 3.6)

# check system is LINUX 
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
    message(STATUS "Build on LINUX")
endif()

if(NOT DEFINED CONFIG_DONE)
	set(CONFIG_DONE ON)
	
	set(freertos "freertos_v202012.00")
	set(lwip "lwip_v2.1.2")
	set(mbedtls "mbedtls-3.0.0")
	
	message(STATUS "FreeRTOS = ${freertos}")
	message(STATUS "LWIP     = ${lwip}")
	message(STATUS "mbedTLS  = ${mbedtls}")
	
	if(NOT DEFINED CUTVER)
		set(CUTVER "B")
	endif()

	if(CUTVER STREQUAL "TEST")
		set(MPCHIP OFF)
	else()
		set(MPCHIP ON)
	endif()

	if(NOT DEFINED BUILD_TZ)
		set(BUILD_TZ OFF)
	endif()

	message(STATUS "MPCHIP ${MPCHIP} : ${CUTVER}-CUT")
	message(STATUS "Build TZ ${BUILD_TZ}")

	# for simulation, not use now
	if(NOT DEFINED BUILD_PXP)
		set(BUILD_PXP OFF)
	endif()

	# for simulation, not use now
	if(NOT DEFINED BUILD_FPGA)
		set(BUILD_FPGA OFF)
	endif()

	if(NOT DEFINED BUILD_LIB)
		set(BUILD_LIB ON)
	endif()
	
	message(STATUS "Build libraries ${BUILD_LIB}")
	message(STATUS "Build FPGA ${BUILD_FPGA}")
	message(STATUS "Build PXP ${BUILD_PXP}")	

	if(NOT DEFINED BUILD_KVS_DEMO)
		set(BUILD_KVS_DEMO OFF)
	endif()

	if(NOT DEFINED DEBUG)
		set(DEBUG OFF)
	endif()
	
	#elf2bin
	if(NOT DEFINED ELF2BIN)
	if(MPCHIP)
		if (LINUX)
		set(ELF2BIN ${prj_root}/GCC-RELEASE/mp/elf2bin.linux)
		else()
		set(ELF2BIN ${prj_root}/GCC-RELEASE/mp/elf2bin.exe)
		endif()
	else()
		if (LINUX)
		set(ELF2BIN ${prj_root}/GCC-RELEASE/elf2bin.linux)
		else()
		set(ELF2BIN ${prj_root}/GCC-RELEASE/elf2bin.exe)
		endif()
	endif()
	endif()		
	
	#chksum
	if(NOT DEFINED CHKSUM)
	if(MPCHIP)
		if (LINUX)
		set(CHKSUM ${prj_root}/GCC-RELEASE/mp/checksum.linux)
		else()		
		set(CHKSUM ${prj_root}/GCC-RELEASE/mp/checksum.exe)
		endif()
	endif()
	endif()		
	
	#default postbuild script
	if (MPCHIP)
		set(POSTBUILD_BOOT		${prj_root}/GCC-RELEASE/mp/amebapro2_bootloader.json)
		set(POSTBUILD_FW_NTZ 	${prj_root}/GCC-RELEASE/mp/amebapro2_firmware_ntz.json)
		set(POSTBUILD_FW_TZ		${prj_root}/GCC-RELEASE/mp/amebapro2_firmware_tz.json)
		set(POSTBUILD_KEY_CFG	${prj_root}/GCC-RELEASE/mp/key_cfg.json)
		set(POSTBUILD_CERT		${prj_root}/GCC-RELEASE/mp/certificate.json)
		set(POSTBUILD_PART		${prj_root}/GCC-RELEASE/mp/amebapro2_partitiontable.json)
		set(POSTBUILD_NNMDL		${prj_root}/GCC-RELEASE/mp/amebapro2_nn_model.json)
		set(POSTBUILD_FWFS_NN	${prj_root}/GCC-RELEASE/mp/amebapro2_fwfs_nn_models.json)
		
		set(POSTBUILD_ENC_BOOT	${prj_root}/GCC-RELEASE/mp/encrypt_bl.json)
		set(POSTBUILD_ENC_NTZ	${prj_root}/GCC-RELEASE/mp/encrypt_fw.json)
		set(POSTBUILD_ENC_TZ	${prj_root}/GCC-RELEASE/mp/encrypt_fw_tz.json)	
		
		set(NN_MODEL_PATH		${prj_root}/src/test_model)
		#set(USED_NN_MODEL		${prj_root}/src/test_model/yolov4_tiny.nb)
		#set(USED_NN_MODEL		${prj_root}/src/test_model/yamnet_fp16.nb)
	endif()	

	execute_process(
		COMMAND
			whoami
		TIMEOUT
			1
		OUTPUT_VARIABLE
			_user_name
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)
	cmake_host_system_information(RESULT _fqdn QUERY FQDN)

	string(TIMESTAMP _configuration_time "%Y-%m-%d %H:%M:%S [UTC]" UTC)
	string(TIMESTAMP _configuration_date "%Y-%m-%d" UTC)

	get_filename_component(_compiler_name ${CMAKE_C_COMPILER} NAME)

	configure_file(${prj_root}/inc/build_info.h.in ${prj_root}/inc/build_info.h @ONLY)

	if(BUILD_PXP)
		message(STATUS "Setup for PXP")
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_PXP.*0/CONFIG_PXP\t\t\t\t\t\t\t1/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_FPGA.*1/CONFIG_FPGA\t\t\t\t\t\t\t0/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_ASIC.*1/CONFIG_ASIC\t\t\t\t\t\t\t0/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
	elseif(BUILD_FPGA)
		message(STATUS "Setup for FPGA")
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_PXP.*1/CONFIG_PXP\t\t\t\t\t\t\t0/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_FPGA.*0/CONFIG_FPGA\t\t\t\t\t\t\t1/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_ASIC.*1/CONFIG_ASIC\t\t\t\t\t\t\t0/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
	else()
		message(STATUS "Setup for ASIC")
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_PXP.*1/CONFIG_PXP\t\t\t\t\t\t\t0/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_FPGA.*1/CONFIG_FPGA\t\t\t\t\t\t\t0/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
		execute_process(COMMAND bash "-c" "sed -i 's/CONFIG_ASIC.*0/CONFIG_ASIC\t\t\t\t\t\t\t1/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
	endif()

	if(CUTVER STREQUAL "TEST" AND MPCHIP)
		message(FATAL_ERROR "MPCHIP cannot be TEST CUT, please check setting")
	endif()

	if(CUTVER STREQUAL "A")
		message(STATUS "Setup for ${CUTVER}-CUT")
		execute_process(COMMAND bash "-c" "sed -i 's/define.*CONFIG_CHIP_VER.*_CUT/define CONFIG_CHIP_VER\t\t\t\t\t\t\tCHIP_A_CUT/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
	elseif(CUTVER STREQUAL "B")
		message(STATUS "Setup for ${CUTVER}-CUT")
		execute_process(COMMAND bash "-c" "sed -i 's/define.*CONFIG_CHIP_VER.*_CUT/define CONFIG_CHIP_VER\t\t\t\t\t\t\tCHIP_B_CUT/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
	elseif(CUTVER STREQUAL "C")
		message(STATUS "Setup for ${CUTVER}-CUT")
		execute_process(COMMAND bash "-c" "sed -i 's/define.*CONFIG_CHIP_VER.*_CUT/define CONFIG_CHIP_VER\t\t\t\t\t\t\tCHIP_C_CUT/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
	elseif (CUTVER STREQUAL "TEST")
		message(STATUS "Setup for ${CUTVER}-CUT")
		execute_process(COMMAND bash "-c" "sed -i 's/define.*CONFIG_CHIP_VER.*_CUT/define CONFIG_CHIP_VER\t\t\t\t\t\t\tCHIP_TEST_CUT/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
		execute_process(COMMAND bash "-c" "sed -i 's/define.*CONFIG_CHIP_VER.*_CUT/define CONFIG_CHIP_VER\t\t\t\t\t\t\tCHIP_TEST_CUT/' ${sdk_root}/component/soc/8735b/cmsis/rtl8735b/include/platform_conf.h" )
	endif()

endif() #CONFIG_DONE

