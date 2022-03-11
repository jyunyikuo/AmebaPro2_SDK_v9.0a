#include "platform_opts.h"
#include "basic_types.h"
#include "stdlib.h"
#include <lwipconf.h>
#include <httpc/httpc.h>

#include "ai_agent_api.h"

#define AIAGENT_SUCCESS 0
#define AIAGENT_ERROR   1

/*--------------------------------------------------------------*/
/*    AI Agent FUNCTION TABLE                                   */
/*--------------------------------------------------------------*/
#define AIAGENT_SDRAM_STUB_SECTION   SECTION(".sdram.aiagent.stubs")
#define MAINAPP_SDRAM_STUB_SECTION   SECTION(".sdram.mainapp.stubs")

AIAGENT_SDRAM_STUB_SECTION
aiAgent_func_stubs_t aiAgent_func_stubs = {
    .__aiAgent_init = aiAgent_init,
    .__aiAgent_main = aiAgent_main,
    .__aiAgent_set_score = aiAgent_set_score,
    .__aiAgent_get_score = aiAgent_get_score
};

MAINAPP_SDRAM_STUB_SECTION
mainApp_func_stubs_t mainApp_func_stubs;


/*-------------------------------------------------------------------------------------------------*/
/*    FUNCTION COMPAT MACRO                                                                        */
/*-------------------------------------------------------------------------------------------------*/
/* std */
#define printf                              mainApp_func_stubs.__printf
#define memset                              mainApp_func_stubs.__memset
 /* freertos */
#define vTaskDelay                          mainApp_func_stubs.__vTaskDelay
#define vTaskDelete                         mainApp_func_stubs.__vTaskDelete
#define xTaskCreate                         mainApp_func_stubs.__xTaskCreate
#define xQueueCreate                        mainApp_func_stubs.__xQueueCreate
#define xQueueReset                         mainApp_func_stubs.__xQueueReset
#define xQueueSend                          mainApp_func_stubs.__xQueueSend
#define xQueueReceive                       mainApp_func_stubs.__xQueueReceive
/* httpc */
#define httpc_conn_new                      mainApp_func_stubs.__httpc_conn_new
#define httpc_conn_connect                  mainApp_func_stubs.__httpc_conn_connect
#define httpc_request_write_header_start    mainApp_func_stubs.__httpc_request_write_header_start
#define httpc_request_write_header          mainApp_func_stubs.__httpc_request_write_header
#define httpc_request_write_header_finish   mainApp_func_stubs.__httpc_request_write_header_finish
#define httpc_response_read_header          mainApp_func_stubs.__httpc_response_read_header
#define httpc_conn_dump_header              mainApp_func_stubs.__httpc_conn_dump_header
#define httpc_response_is_status            mainApp_func_stubs.__httpc_response_is_status
#define httpc_response_read_data            mainApp_func_stubs.__httpc_response_read_data
#define httpc_conn_close                    mainApp_func_stubs.__httpc_conn_close
#define httpc_conn_free                     mainApp_func_stubs.__httpc_conn_free
/*-------------------------------------------------------------------------------------------------*/

static int aiAgent_score = 0;
xQueueHandle aiAgentQueue;
#define SERVER_HOST  "httpbin.org"

#define TEST_HTTPS_DEMO 1
#define TEST_RTOS_API   1
#define TEST_INTERNAL   1

void aiAgent_init(mainApp_func_stubs_t main_func_stubs)
{
    /* aiAgent init here.
    ** If something need to do before entering aiAgent_main.
    */
    
}

void aiAgent_thread(void *param)
{
    printf("aiAgent_thread start!\r\n");
    vTaskDelete(NULL);
}

int aiAgent_main(void)
{
    printf("== aiAgent main ==\r\n");
    
    int ret = AIAGENT_SUCCESS;
    
    /*-----------------------------------------------------*/
    /*--------- HTTPS TEST --------------------------------*/
    /*-----------------------------------------------------*/
#if TEST_HTTPS_DEMO
    struct httpc_conn *conn = NULL;

	printf("\nExample: HTTPC\n");
    conn = httpc_conn_new(HTTPC_SECURE_TLS, NULL, NULL, NULL);
    if (conn) {
        if (httpc_conn_connect(conn, SERVER_HOST, 443, 0) == 0) {
            /* HTTP GET request */
			// start a header and add Host (added automatically), Content-Type and Content-Length (added by input param)
			httpc_request_write_header_start(conn, "GET", "/get?param1=test_data1&param2=test_data2", NULL, 0);
			// add other required header fields if necessary
			httpc_request_write_header(conn, "Connection", "close");
			// finish and send header
			httpc_request_write_header_finish(conn);

			// receive response header
			if (httpc_response_read_header(conn) == 0) {
				httpc_conn_dump_header(conn);

				// receive response body
				if (httpc_response_is_status(conn, "200 OK")) {
					uint8_t buf[1024];
					int read_size = 0;
					uint32_t total_size = 0;

					while (1) {
						memset(buf, 0, sizeof(buf));
						read_size = httpc_response_read_data(conn, buf, sizeof(buf) - 1);

						if (read_size > 0) {
							total_size += read_size;
							printf("%s", buf);
						} else {
							break;
						}

						if (conn->response.content_len && (total_size >= conn->response.content_len)) {
							break;
						}
					}
				}
			}
		} else {
			printf("\nERROR: httpc_conn_connect\n");
		}

		httpc_conn_close(conn);
		httpc_conn_free(conn);
	}
#endif


    /*-----------------------------------------------------*/
    /*--------- FreeRTOS API TEST -------------------------*/
    /*-----------------------------------------------------*/
#if TEST_RTOS_API
    /* create task test */
    if (xTaskCreate(aiAgent_thread, ((const char *)"aiAgent_thread"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n aiAgent_thread: Create Task Error\r\n");
    }
    else {
        printf("\r\n aiAgent_thread: Create Task Success!\r\n");
    }
    /* create queue test */
    aiAgentQueue = xQueueCreate(20, sizeof(int));
	xQueueReset(aiAgentQueue);
    int tmp_item = 10;
    xQueueSend(aiAgentQueue, &tmp_item, 0);
    tmp_item = 20;
    xQueueSend(aiAgentQueue, &tmp_item, 0);
    xQueueReceive(aiAgentQueue, &tmp_item, 0);
    printf("xQueueReceive:%d\r\n", tmp_item);
    xQueueReceive(aiAgentQueue, &tmp_item, 0);
    printf("xQueueReceive:%d\r\n", tmp_item);
#endif
    
    
    /*-----------------------------------------------------*/
    /*--------- AI AGENT INTERNAL API TEST ----------------*/
    /*-----------------------------------------------------*/
#if TEST_INTERNAL
    int c_tmp = 100;
    aiAgent_set_score(c_tmp);
    printf("aiAgent set score to:%d\r\n", c_tmp);
    c_tmp = aiAgent_get_score();
    printf("aiAgent get score:%d\r\n", c_tmp);
#endif
    
    return ret;
}

void aiAgent_set_score(int score)
{
    printf("aiAgent_set_score...\r\n");
    aiAgent_score = score;
}

int aiAgent_get_score(void)
{
    printf("aiAgent_get_score...\r\n");
    return aiAgent_score;
}

