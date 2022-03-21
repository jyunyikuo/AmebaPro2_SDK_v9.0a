#include <platform_opts.h>
#include "FreeRTOS.h"
#include "task.h"

static void example_cpp_test_thread(void)
{
    printf("=== example_cpp_test START ===\r\n");
    
	test1();
    test2();
    test3();
    test4();
    //test5("Ameba: Hello!");
    
    printf("=== example_cpp_test END ===\r\n");

	vTaskDelete(NULL);
	return;
}

void example_cpp_test(void)
{
	if (xTaskCreate(example_cpp_test_thread, ((const char *)"example_cpp_test_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\n\r%s xTaskCreate(example_cpp_test_thread) failed", __FUNCTION__);
	}
}
