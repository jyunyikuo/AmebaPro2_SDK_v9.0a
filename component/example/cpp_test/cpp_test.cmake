### add linked library ###
list(
    APPEND app_example_lib
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

    ${sdk_root}/component/example/cpp_test/app_example.c
    ${sdk_root}/component/example/cpp_test/example_cpp_test.c

    #src
    ${sdk_root}/component/example/cpp_test/test.cpp
)
