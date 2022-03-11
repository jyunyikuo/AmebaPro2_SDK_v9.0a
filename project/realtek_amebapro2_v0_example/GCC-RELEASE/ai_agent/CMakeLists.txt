cmake_minimum_required(VERSION 3.6)

project(ai_agent)

# root of realtek_amebapro2_v0_example
set(prj_root "${CMAKE_CURRENT_SOURCE_DIR}/../..")
# root of SDK
set(sdk_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
set(ai_agent ai_agent)

include(../config.cmake)

#AI Agent
list(
    APPEND ai_agent_sources

    ${prj_root}/src/ai_agent_src/ai_agent_api.c
)

add_executable(
    ${ai_agent}
    ${ai_agent_sources}
)

list(
    APPEND ai_agent_flags
    CONFIG_BUILD_BOOT=1
    CONFIG_BUILD_RAM=1 
    CONFIG_BUILD_LIB=1 
    CONFIG_PLATFORM_8735B
    CONFIG_RTL8735B_PLATFORM=1
)

target_compile_definitions(${ai_agent} PRIVATE ${ai_agent_flags} )

include(../includepath.cmake)
target_include_directories(
    ${ai_agent}
    PUBLIC

    ${inc_path}
    ${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33_NTZ/non_secure
)

if(NOT PICOLIBC)
target_link_libraries(
    ${ai_agent} 
    nosys
)
endif()

set(ld_script "${CMAKE_CURRENT_SOURCE_DIR}/rtl8735b_ai_agent.ld")
target_link_options(
    ${ai_agent} 
    PUBLIC
    "LINKER:SHELL:-L ${CMAKE_CURRENT_SOURCE_DIR}/../ROM/GCC"
    "LINKER:SHELL:-T ${ld_script}"
    "LINKER:SHELL:-Map=ai_agent.map"
)

if (LINUX)
set(ELF2BIN ${prj_root}/GCC-RELEASE/mp/elf2bin.linux)
else()
set(ELF2BIN ${prj_root}/GCC-RELEASE/mp/elf2bin.exe)
endif()

set_target_properties(${ai_agent} PROPERTIES LINK_DEPENDS ${ld_script})
set_target_properties(${ai_agent} PROPERTIES LINK_DEPENDS ${ELF2BIN})

add_custom_command(TARGET ${ai_agent} POST_BUILD 

    COMMAND ${CMAKE_NM} $<TARGET_FILE:${ai_agent}> | sort > ${ai_agent}.nm.map
    COMMAND ${CMAKE_OBJEDUMP} -d $<TARGET_FILE:${ai_agent}> > ${ai_agent}.asm
    COMMAND cp $<TARGET_FILE:${ai_agent}> ${ai_agent}.axf

    COMMAND cp ${POSTBUILD_AIAGENT} amebapro2_ai_agent.json
    COMMAND ${ELF2BIN} convert amebapro2_ai_agent.json FIRMWARE ai_agent.bin

    COMMAND rm -rf output && mkdir output
    COMMAND cp ai_agent.bin output
    COMMAND cp ${ai_agent}.nm.map output
    COMMAND cp ${ai_agent}.asm output
    COMMAND cp ${ai_agent}.axf output
)
