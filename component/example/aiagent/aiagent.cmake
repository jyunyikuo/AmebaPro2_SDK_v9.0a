### include .cmake need if neeeded ###
include(${sdk_root}/component/example/aiagent/libaiagent.cmake)

### add linked library ###
list(
    APPEND app_example_lib
    aiagent
)

### add flags ###
list(
	APPEND app_example_flags
)

### add header files ###
list (
    APPEND app_example_inc_path
)

### add source file ###
list(
	APPEND app_example_sources

    ${sdk_root}/component/example/aiagent/app_example.c
    ${sdk_root}/component/example/aiagent/example_aiagent.c 
)
