/******************************************************************************
 *
 * Copyright(c) 2007 - 2016 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/
//#include <platform_opts.h>
#include "FreeRTOS.h"
#include "task.h"
//#include <platform/platform_stdlib.h>
#include "semphr.h"
#include "device.h"
#include "serial_api.h"
#include "log_service.h"
#include "osdep_service.h"
#include "serial_ex_api.h"
#include "pinmap.h"

#include "cmsis.h"
#include "mpu.h"
#include "rtl8735b_ramstart.h"
//#include "shell.h"
#include "cmsis_os.h"               // CMSIS RTOS header file
#include "hal.h"
//#include "memory.h"
//#include "diag.h"

#include "stdio_port_func.h"

extern hal_uart_adapter_t log_uart;

#define KEY_CTRL_D		0x4
#define KEY_NL			0xa // '\n'
#define KEY_ENTER		0xd // '\r'
#define KEY_BS			0x8
#define KEY_ESC			0x1B
#define KEY_LBRKT		0x5B
#define STR_END_OF_MP_FORMAT	"\r\n\r\r#"


#define CMD_HISTORY_LEN	4	// max number of executed command saved
extern char log_buf[LOG_SERVICE_BUFLEN];
extern xSemaphoreHandle log_rx_interrupt_sema;
char cmd_history[CMD_HISTORY_LEN][LOG_SERVICE_BUFLEN];
static unsigned int cmd_history_count = 0;

void uart_put_char(u8 c)
{
	//serial_putc(&loguart_sobj, c);
	stdio_port_putc(c);
}

void uart_send_string(char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		//serial_putc(&loguart_sobj, str[i]);
		stdio_port_putc(str[i]);
		i++;
	}
}

void uart_send_buf(u8 *buf, u32 len)
{
	unsigned char *st_p = buf;
	if (!len || (!buf)) {
		return;
	}
	while (len) {
		//serial_putc(&loguart_sobj, *st_p);
		stdio_port_putc(*st_p);
		st_p++;
		len--;
	}
}

_WEAK void uart_set_dbgmon_pending() {}

void uart_irq(u32 id, u32 event)
{
	unsigned char rc = 0;
	static unsigned char temp_buf[LOG_SERVICE_BUFLEN] = "\0";
	static unsigned char combo_key = 0;
	static short buf_count = 0;
	static unsigned char key_enter = 0;
	static char cmd_history_index = 0;
	if (event == RxIrq) {
		//rc = serial_getc(sobj);
		stdio_port_getc((char *)&rc);

		if (key_enter && rc == KEY_NL) {
			//serial_putc(sobj, rc);
			return;
		}

		if (rc == KEY_ESC) {
			combo_key = 1;
		} else if (rc == KEY_CTRL_D) {
			uart_set_dbgmon_pending();
		} else if (combo_key == 1) {
			if (rc == KEY_LBRKT) {
				combo_key = 2;
			} else {
				combo_key = 0;
			}
		} else if (combo_key == 2) {
			if (rc == 'A' || rc == 'B') { // UP or Down
				if (rc == 'A') {
					cmd_history_index--;
					if (cmd_history_index < 0) {
						cmd_history_index = (cmd_history_count > CMD_HISTORY_LEN) ? CMD_HISTORY_LEN - 1 : (cmd_history_count - 1) % CMD_HISTORY_LEN;
					}
				} else {
					cmd_history_index++;
					if (cmd_history_index > (cmd_history_count > CMD_HISTORY_LEN ? CMD_HISTORY_LEN - 1 : (cmd_history_count - 1) % CMD_HISTORY_LEN)) {
						cmd_history_index = 0;
					}
				}

				if (cmd_history_count > 0) {
					buf_count = strlen((char const *)temp_buf);
					memset(temp_buf, '\0', buf_count);
					while (--buf_count >= 0) {
						//serial_putc(sobj, KEY_BS);
						//serial_putc(sobj, ' ');
						//serial_putc(sobj, KEY_BS);
						stdio_port_putc(KEY_BS);
						stdio_port_putc(' ');
						stdio_port_putc(KEY_BS);
					}
					uart_send_string(cmd_history[cmd_history_index % CMD_HISTORY_LEN]);
					strcpy((char *)temp_buf, cmd_history[cmd_history_index % CMD_HISTORY_LEN]);
					buf_count = strlen((char const *)temp_buf);
				}
			}

			// exit combo
			combo_key = 0;
		} else if (rc == KEY_ENTER) {
			key_enter = 1;
			if (buf_count > 0) {
				//serial_putc(sobj, KEY_NL);
				//serial_putc(sobj, KEY_ENTER);
				stdio_port_putc(KEY_NL);
				stdio_port_putc(KEY_ENTER);
				memset(log_buf, '\0', LOG_SERVICE_BUFLEN);
				strncpy(log_buf, (char *)&temp_buf[0], buf_count);
				rtw_up_sema_from_isr(&log_rx_interrupt_sema);
				memset(temp_buf, '\0', buf_count);

				/* save command */
				memset(cmd_history[((cmd_history_count) % CMD_HISTORY_LEN)], '\0', buf_count + 1);
				strcpy(cmd_history[((cmd_history_count++) % CMD_HISTORY_LEN)], log_buf);
				cmd_history_index = cmd_history_count % CMD_HISTORY_LEN;
				//cmd_history_count++;
				buf_count = 0;
			} else {
				uart_send_string(STR_END_OF_MP_FORMAT);
			}
		} else if (rc == KEY_BS) {
			if (buf_count > 0) {
				buf_count--;
				temp_buf[buf_count] = '\0';

				//serial_putc(sobj, rc);
				//serial_putc(sobj, ' ');
				//serial_putc(sobj, rc);
				stdio_port_putc(rc);
				stdio_port_putc(' ');
				stdio_port_putc(rc);
			}
		} else {
			/* cache input characters */
			if (buf_count < (LOG_SERVICE_BUFLEN - 1)) {
				temp_buf[buf_count] = rc;
				buf_count++;
				//serial_putc(sobj, rc);
				stdio_port_putc(rc);
				key_enter = 0;
			} else if (buf_count == (LOG_SERVICE_BUFLEN - 1)) {
				temp_buf[buf_count] = '\0';

				uart_send_string("\r\nERROR:exceed size limit"STR_END_OF_ATCMD_RET);
			}
		}
	}
}

extern void log_service_init(void);
void console_init(void)
{
	int i = 0;
	hal_uart_rxind_hook(&log_uart, uart_irq, 0, 0);
	for (i = 0; i < CMD_HISTORY_LEN; i++) {
		memset(cmd_history[i], '\0', LOG_SERVICE_BUFLEN);
	}
	log_service_init();
}

void console_reinit_uart(void)
{
	hal_uart_reset_rx_fifo(&log_uart);
	hal_uart_rxind_hook(&log_uart, uart_irq, 0, 0);
}



