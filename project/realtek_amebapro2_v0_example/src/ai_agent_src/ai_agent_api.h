#ifndef _AI_AGENT_API_H_
#define _AI_AGENT_API_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef struct aiAgent_func_stubs_s {
    
	void (*__aiAgent_init)(void);
    int (*__aiAgent_main)(void);
    void (*__aiAgent_set_score)(int);
    int (*__aiAgent_get_score)(void);

} aiAgent_func_stubs_t;

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

/*-------------------------------------------*/

void aiAgent_init(mainApp_func_stubs_t);

int aiAgent_main(void);

void aiAgent_set_score(int);

int aiAgent_get_score(void);

#endif /* _AI_AGENT_API_H_ */