/******************************************************************************
*
* Copyright(c) 2007 - 2021 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <httpc/httpc.h>
#include <hal_cache.h>
#include "video_example_media_framework.h"
#include "ai_agent_api.h"
#include "fwfs.h"

extern uint8_t __eram2_start__[];
#define AI_AGENT_ADDR   (uint32_t)__eram2_start__

// most queue api are marco
QueueHandle_t __xQueueCreate( UBaseType_t a, UBaseType_t b)
{
	return xQueueCreate(a, b);
}
BaseType_t __xQueueReset( QueueHandle_t a)
{
	return xQueueReset(a);
}

BaseType_t __xQueueSend( QueueHandle_t a, const void * b, TickType_t c)
{
	return xQueueSend(a,b,c);
}

BaseType_t __xQueueReceive( QueueHandle_t a, void * b, TickType_t c)
{
	return xQueueReceive(a,b,c);
}

static mainApp_func_stubs_t mainapp_stubs = 
{
	printf,
	memset,
	vTaskDelay,
	vTaskDelete,
	xTaskCreate,
	__xQueueCreate,
	__xQueueReset,
	__xQueueSend,
	__xQueueReceive,
	httpc_conn_new,
	httpc_conn_connect,
	httpc_request_write_header_start,
	httpc_request_write_header,
	httpc_request_write_header_finish,
	httpc_response_read_header,
	httpc_conn_dump_header,
	httpc_response_is_status,
	httpc_response_read_data,
	httpc_conn_close,
	httpc_conn_free
};

void ai_agent_task(void* arg)
{
	printf("Enter ai_agent_task...\n\r");
	aiAgent_func_stubs_t *stubs = (aiAgent_func_stubs_t *)arg;

	stubs->__aiAgent_main();

	printf("mainapp aiagent stop!\r\n");
	vTaskDelete(NULL);
}

void mmf2_video_ai_agent_init(void)
{
	printf("== mmf2_video_ai_agent_init demo start ==\r\n");

	void *fp = pfw_open("MP", M_NORMAL);
	pfw_seek(fp, 0, SEEK_END);
	int len = pfw_tell(fp);
	pfw_seek(fp, 128, SEEK_SET);
	printf("ai agent size %d\n\r", len);
	printf("AI_AGENT_ADDR %lu\n\r", AI_AGENT_ADDR);

	int rd_status = pfw_read(fp, (void*)AI_AGENT_ADDR, len);
	pfw_close(fp);
	//pfw_dump_mem((void*)AI_AGENT_ADDR, 256);

	// init function pointer - exchange function table 
	aiAgent_func_stubs_t *aiagent_stubs = (aiAgent_func_stubs_t *)AI_AGENT_ADDR;
	mainApp_func_stubs_t *aiagent_mainapp_stubs = (mainApp_func_stubs_t *)(AI_AGENT_ADDR+sizeof(aiAgent_func_stubs_t));

	memcpy(aiagent_mainapp_stubs, &mainapp_stubs, sizeof(mainApp_func_stubs_t));

	dcache_clean_by_addr((uint32_t *)AI_AGENT_ADDR, len+31);
	icache_invalidate();

	//pfw_dump_mem((void*)AI_AGENT_ADDR, 256);

	// create task
	printf("creating ai_agent_task...\n\r");
	xTaskCreate(ai_agent_task, "aiagent", 4096, (void*)aiagent_stubs, 1, NULL);

	return;
mmf2_video_ai_agent_fail:

	return;
}