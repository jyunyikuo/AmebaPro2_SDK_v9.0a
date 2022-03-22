cmake_minimum_required(VERSION 3.6)

project(aiagent)

set(aiagent aiagent)

list(
    APPEND aiagent_sources

    #src
    ${sdk_root}/component/example/aiagent/aiagent_src/agent_test.cpp

)

add_library(
    ${aiagent} STATIC
    ${aiagent_sources}
)

list(
	APPEND aiagent_flags
	CONFIG_BUILD_RAM=1 
	CONFIG_BUILD_LIB=1 
	CONFIG_PLATFORM_8735B
	CONFIG_RTL8735B_PLATFORM=1
	CONFIG_SYSTEM_TIME64=1
	ARM_MATH_ARMV8MML
)

target_compile_definitions(${aiagent} PRIVATE ${aiagent_flags} )

include(../includepath.cmake)
target_include_directories(
	${aiagent}
	PUBLIC

	${inc_path}
	${sdk_root}/component/os/freertos/${freertos}/Source/portable/GCC/ARM_CM33_NTZ/non_secure

)
