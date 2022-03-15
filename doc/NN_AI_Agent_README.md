# NN AI Agent - Quick Start

This example can seperate part of your application code from main app, and build it as another firmware binary(Agent.bin). The example just show the concept of how to link the function pointer between two independent firmware binary.

User can only update this part of application code by download a newer Agent.bin to a specified flash region.

## Application Code of Main App and Agent

### The App Code of Agent

We provide the HTTPS demo in Agent main function - aiAgent_main (`project/realtek_amebapro2_v0_example/src/ai_agent_src/ai_agent_api.c`)
user can do their work here:
```
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
    ...
    ...
}
```

### The Code of Main App

We give a simple task to call aiAgent_main in main app (`project/realtek_amebapro2_v0_example/src/mmfv2_video_example/mmf2_video_ai_agent_init.c`)
```
void ai_agent_task(void* arg)
{
    printf("Enter ai_agent_task...\n\r");
    aiAgent_func_stubs_t *stubs = (aiAgent_func_stubs_t *)arg;

    stubs->__aiAgent_main();

    printf("mainapp aiagent stop!\r\n");
    vTaskDelete(NULL);
}
```

## Build the Function Table

### The Agent API will be called by Main App

user should define them in `project/realtek_amebapro2_v0_example/src/ai_agent_src/ai_agent_api.h`

```
typedef struct aiAgent_func_stubs_s {
    
    void (*__aiAgent_init)(void);
    int (*__aiAgent_main)(void);
    void (*__aiAgent_set_score)(int);
    int (*__aiAgent_get_score)(void);

} aiAgent_func_stubs_t;
```

### The SDK Function will be used in Agent

user should define them in `project/realtek_amebapro2_v0_example/src/ai_agent_src/ai_agent_api.h
`  

```
typedef struct mainApp_func_stubs_s {

    /* std */
    int (*__printf)( const char *, ... );
    void * (*__memset)( void * ptr, int value, size_t num );

    /* freertos */
    void (*__vTaskDelay)( const TickType_t);
    void (*__vTaskDelete)( TaskHandle_t );
    BaseType_t (*__xTaskCreate)( TaskFunction_t, const char * const, configSTACK_DEPTH_TYPE, void *, UBaseType_t, TaskHandle_t * );
    QueueHandle_t (*__xQueueCreate)( UBaseType_t, UBaseType_t );
    BaseType_t (*__xQueueReset)( QueueHandle_t );
    BaseType_t (*__xQueueSend)( QueueHandle_t, const void *, TickType_t );
    BaseType_t (*__xQueueReceive)( QueueHandle_t, void *, TickType_t );

    /* httpc */
    struct httpc_conn *(*__httpc_conn_new)(uint8_t secure, char *client_cert, char *client_key, char *ca_certs);
    int (*__httpc_conn_connect)(struct httpc_conn *conn, char *host, uint16_t port, uint32_t timeout);
    int (*__httpc_request_write_header_start)(struct httpc_conn *conn, char *method, char *resource, char *content_type, size_t content_len);
    int (*__httpc_request_write_header)(struct httpc_conn *conn, char *name, char *value);
    int (*__httpc_request_write_header_finish)(struct httpc_conn *conn);
    int (*__httpc_response_read_header)(struct httpc_conn *conn);
    void (*__httpc_conn_dump_header)(struct httpc_conn *conn);
    int (*__httpc_response_is_status)(struct httpc_conn *conn, char *status);
    int (*__httpc_response_read_data)(struct httpc_conn *conn, uint8_t *data, size_t data_len);
    void (*__httpc_conn_close)(struct httpc_conn *conn);
    void (*__httpc_conn_free)(struct httpc_conn *conn);

} mainApp_func_stubs_t;
```

For the function name compatable, we can define them by  
```
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
```

## Exchange Function Table in Main App before call the aiAgent_main

### Load Agent Firmware to RAM
```
void *fp = pfw_open("MP", M_NORMAL);
pfw_seek(fp, 0, SEEK_END);
int len = pfw_tell(fp);
pfw_seek(fp, 128, SEEK_SET);
printf("ai agent size %d\n\r", len);
printf("AI_AGENT_ADDR %lu\n\r", AI_AGENT_ADDR);

int rd_status = pfw_read(fp, (void*)AI_AGENT_ADDR, len);
pfw_close(fp);
```

### Init Function Pointer - Exchange Function Table 
```
aiAgent_func_stubs_t *aiagent_stubs = (aiAgent_func_stubs_t *)AI_AGENT_ADDR;
mainApp_func_stubs_t *aiagent_mainapp_stubs = (mainApp_func_stubs_t *)(AI_AGENT_ADDR+sizeof(aiAgent_func_stubs_t));

memcpy(aiagent_mainapp_stubs, &mainapp_stubs, sizeof(mainApp_func_stubs_t));

dcache_clean_by_addr((uint32_t *)AI_AGENT_ADDR, len+31);
icache_invalidate();
```

## Build image

To build the example run the following command:
```
cd <AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -DVIDEO_EXAMPLE=ON -DDEBUG=ON
cmake --build . --target flash -j4
```
The main app image is located in `<AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE/build/flash_ntz.bin`

The Agent app image is located in `<AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE/build/ai_agent.bin`

## Download Image

Make sure your AmebaPro2 is connected and powered on. Use the Realtek image tool to flash image:

- download main app image - flash_ntz.bin with offset 0
- download Agent app image - ai_agent.bin with offset 0xD60000

## Validation

### Check Log

Reboot your device and check the logs.

You can choose different demo in Agent, and re-build a ai_agent.bin
```
#define TEST_HTTPS_DEMO 1
#define TEST_RTOS_API   1
#define TEST_INTERNAL   1
```
Then, just download ai_agent.bin with offset 0xD60000 to update Agent application code!