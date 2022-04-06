#include <platform_opts.h>
#include "FreeRTOS.h"
#include "task.h"

#include "wifi_conf.h"
#include "lwip_netconf.h"

#define wifi_wait_time 500
static void wifi_common_init()
{
	uint32_t wifi_wait_count = 0;

	while (!((wifi_get_join_status() == RTW_JOINSTATUS_SUCCESS) && (*(u32 *)LwIP_GetIP(0) != IP_ADDR_INVALID))) {
		vTaskDelay(10);
		wifi_wait_count++;
		if (wifi_wait_count == wifi_wait_time) {
			printf("\r\nuse ATW0, ATW1, ATWC to make wifi connection\r\n");
			printf("wait for wifi connection...\r\n");
		}
	}

}

static void example_aiagent_thread(void)
{
    wifi_common_init();
    
    printf("=== AI AGENT TEST START ===\r\n");
    
    test1();
    test2();
    test3();
    test4();
    //test5("Ameba: Hello!");
    test6();
    
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
