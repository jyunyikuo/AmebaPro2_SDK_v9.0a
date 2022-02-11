cmake_minimum_required(VERSION 3.6)

project(flash_bin)

# root of realtek_amebapro2_v0_example
set (prj_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
# root of SDK
set (sdk_root "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

include(./config.cmake)

#add_subdirectory (bootloader)

if(BUILD_TZ)
	if(BUILD_WLANMP)
		set(target flash_tz_mp)
	else()
		set(target flash_tz)
	endif()
	set(fw application/firmware_tz.bin)
	set(app_dep application.s application.ns)
	
	set(bttrace application/APP.trace)
	add_subdirectory (application)
else()
	if(BUILD_WLANMP)
		set(target flash_ntz_mp)
	else()
		set(target flash_ntz)
	endif()
	set(fw application/firmware_ntz.bin)	
	set(app_dep application.ntz)
	
	set(bttrace application/APP.trace)
	add_subdirectory (application)
endif()

if(MPCHIP)
add_custom_command(
	OUTPUT key_public.json key_private.json
	
	COMMAND cp ${POSTBUILD_KEY_CFG} keycfg.json
	COMMAND ${ELF2BIN} keygen keycfg.json key
)

add_custom_target(
	keygen
	
	COMMAND cp ${POSTBUILD_KEY_CFG} keycfg.json
	COMMAND ${ELF2BIN} keygen keycfg.json key
)

add_custom_target(
	plain_img

	COMMAND cp ${POSTBUILD_CERT} certificate.json
	COMMAND ${ELF2BIN} convert certificate.json CERT_TABLE certable.bin	
	COMMAND ${ELF2BIN} convert certificate.json CERTIFICATE certificate.bin	
	
	#COMMAND cp ${POSTBUILD_KEY_CFG} keycfg.json
	#COMMAND ${ELF2BIN} keygen keycfg.json key

	COMMAND cp ${POSTBUILD_PART} amebapro2_partitiontable.json
	COMMAND ${ELF2BIN} convert amebapro2_partitiontable.json PARTITIONTABLE partition.bin
	
	COMMAND cp ${prj_root}/GCC-RELEASE/bootloader/boot.bin .
	COMMAND cp ${fw} ./firmware.bin

	COMMAND cp ${bttrace} ./APP.trace
	
	#DEPENDS bootloader
	DEPENDS ${app_dep}
	DEPENDS ${POSTBUILD_CERT}
	DEPENDS ${POSTBUILD_KEY_CFG}
	DEPENDS ${POSTBUILD_PART}
	DEPENDS key_public.json
	DEPENDS key_private.json
)

add_custom_target(
	flash

	COMMAND ${ELF2BIN} combine amebapro2_partitiontable.json ${target}.bin PT_PT=partition.bin,CER_TBL=certable.bin,KEY_CER1=certificate.bin,PT_BL_PRI=boot.bin,PT_FW1=firmware.bin
	COMMAND cp ./firmware.bin ./ota.bin
	COMMAND ${CHKSUM} ./ota.bin
	
	DEPENDS plain_img
)
	
add_custom_target(
	flash_nn
	COMMAND cp ${USED_NN_MODEL} model.nb
	COMMAND cp ${POSTBUILD_NNMDL} amebapro2_nn_model.json
	COMMAND ${ELF2BIN} convert amebapro2_nn_model.json FIRMWARE nn_model.bin
	COMMAND ${ELF2BIN} combine amebapro2_partitiontable.json ${target}.nn.bin PT_PT=partition.bin,CER_TBL=certable.bin,KEY_CER1=certificate.bin,PT_BL_PRI=boot.bin,PT_FW1=firmware.bin,PT_NN_MDL=nn_model.bin
	
	DEPENDS plain_img
)

	
add_custom_target(
	hash

	COMMAND ${ELF2BIN} secure sign+dbg=cert key_private.json key_public.json certificate.bin certificate_signed.bin
	COMMAND ${ELF2BIN} secure hash+dbg=ptab key_private.json key_public.json partition.bin partition_hashed.bin
	COMMAND ${ELF2BIN} secure hash+dbg=boot key_private.json key_public.json boot.bin boot_hashed.bin
	COMMAND ${ELF2BIN} secure hash+dbg=fw key_private.json key_public.json firmware.bin firmware_hashed.bin
	
	
	COMMAND ${ELF2BIN} combine amebapro2_partitiontable.json ${target}.bin PT_PT=partition_hashed.bin,CER_TBL=certable.bin,KEY_CER1=certificate_signed.bin,PT_BL_PRI=boot_hashed.bin,PT_FW1=firmware_hashed.bin
	COMMAND cp ./firmware_hashed.bin ./ota.bin
	COMMAND ${CHKSUM} ./ota.bin
	
	DEPENDS plain_img
)	

add_custom_target(
	sign

	COMMAND ${ELF2BIN} secure sign+dbg=cert key_private.json key_public.json certificate.bin certificate_signed.bin
	COMMAND ${ELF2BIN} secure sign+hash+dbg=ptab key_private.json key_public.json partition.bin partition_signed.bin
	COMMAND ${ELF2BIN} secure sign+hash+dbg=boot key_private.json key_public.json boot.bin boot_signed.bin
	COMMAND ${ELF2BIN} secure sign+hash+dbg=fw key_private.json key_public.json firmware.bin firmware_signed.bin
	
	
	COMMAND ${ELF2BIN} combine amebapro2_partitiontable.json ${target}.bin PT_PT=partition_signed.bin,CER_TBL=certable.bin,KEY_CER1=certificate_signed.bin,PT_BL_PRI=boot_signed.bin,PT_FW1=firmware_signed.bin
	COMMAND cp ./firmware_signed.bin ./ota.bin
	COMMAND ${CHKSUM} ./ota.bin
	
	DEPENDS plain_img
)

add_custom_target(
	sign_enc
	
	COMMAND ${ELF2BIN} secure sign+dbg=cert key_private.json key_public.json certificate.bin certificate_signed.bin
	COMMAND ${ELF2BIN} secure sign+hash+dbg=ptab key_private.json key_public.json partition.bin partition_signed.bin
	COMMAND ${ELF2BIN} secure sign+enc+hash+dbg=boot key_private.json key_public.json ${POSTBUILD_ENC_BOOT}  boot.bin boot_signed_enc.bin
	COMMAND ${ELF2BIN} secure sign+enc+hash+dbg=fw key_private.json key_public.json ${POSTBUILD_ENC_NTZ} firmware.bin firmware_signed_enc.bin
	
	
	COMMAND ${ELF2BIN} combine amebapro2_partitiontable.json ${target}.bin PT_PT=partition_signed.bin,CER_TBL=certable.bin,KEY_CER1=certificate_signed.bin,PT_BL_PRI=boot_signed_enc.bin,PT_FW1=firmware_signed_enc.bin
	COMMAND cp ./firmware_signed_enc.bin ./ota.bin
	COMMAND ${CHKSUM} ./ota.bin
	
	DEPENDS plain_img
)
else()
add_custom_target(
	flash
	
	COMMAND cp ${prj_root}/GCC-RELEASE/keycfg.json keycfg.json
	COMMAND cp ${prj_root}/GCC-RELEASE/partition.json partition.json
	COMMAND cp ${prj_root}/GCC-RELEASE/amebapro2_partition.json amebapro2_partition.json
	COMMAND ${ELF2BIN} keygen keycfg.json
	
	COMMAND ${ELF2BIN} convert amebapro2_partition.json PARTITIONTABLE 
	COMMAND cp ${prj_root}/GCC-RELEASE/bootloader/boot.bin .
	
	COMMAND cp ${fw} ./firmware.bin
	COMMAND ${ELF2BIN} combine ${target}.bin PTAB=partition.bin,BOOT=boot.bin,FW1=firmware.bin
	
	COMMAND cp ${bttrace} ./APP.trace
	
	#DEPENDS bootloader
	DEPENDS ${app_dep}
)
endif()
