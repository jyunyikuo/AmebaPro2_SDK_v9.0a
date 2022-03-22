#include <platform_opts.h>
#include "FreeRTOS.h"
#include "task.h"

static void example_aiagent_thread(void)
{
    printf("=== AI AGENT TEST START ===\r\n");
    
    test1();
    test2();
    test3();
    test4();
    //test5("Ameba: Hello!");
    
    printf("=== AI AGENT TEST END ===\r\n");

	vTaskDelete(NULL);
	return;
}

void example_aiagent(void)
{
	if (xTaskCreate(example_aiagent_thread, ((const char *)"example_aiagent_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\n\r%s xTaskCreate(example_aiagent_thread) failed", __FUNCTION__);
	}
}
