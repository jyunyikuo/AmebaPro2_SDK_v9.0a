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
)

target_compile_definitions(${aiagent} PRIVATE ${aiagent_flags} )


target_include_directories(
	${aiagent}
	PUBLIC
)
