#include <platform_stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "serial_api.h"
#include "semphr.h"
#include "log_service.h"
#include <stdbool.h>
#include "osdep_service.h"
#include "platform_opts.h"

//#define CONFIG_HCI_UART_BRIDGE_TO_LOG_UART		1

#ifndef CONFIG_HCI_UART_BRIDGE_TO_LOG_UART
#if defined(CONFIG_ATCMD_MP) && CONFIG_ATCMD_MP
#define CONFIG_HCI_UART_BRIDGE_TO_LOG_UART		1		// In AmebaPro2 MP test, use Log Uart as HCI Uart
#endif
#endif

#if defined(CONFIG_HCI_UART_BRIDGE_TO_LOG_UART) && CONFIG_HCI_UART_BRIDGE_TO_LOG_UART

#define KEY_ENTER		0xd		//'\r'

extern char log_buf[LOG_SERVICE_BUFLEN];
extern hal_uart_adapter_t log_uart;
extern xSemaphoreHandle log_rx_interrupt_sema;

static uint32_t uart_baudrate = 0;
static uint8_t uart_parity = 0;
static uint8_t check_byte_num = 0;
static int log_flag = 0;

extern void amebapro2_uart_bridge_open(bool flag);
extern void amebapro2_uart_bridge_to_hci(uint8_t rc);
extern uint32_t btc_set_single_tone_tx(uint8_t bStart);

void bt_uart_bridge_putc(uint8_t tx_data)
{
	while (!hal_uart_writeable(&log_uart));
		hal_uart_putc(&log_uart, tx_data);
}

uint8_t bt_uart_bridge_getc(void)
{
	while (!hal_uart_readable(&log_uart));
	return (uint8_t)(hal_uart_getc(&log_uart));
}

static void bt_uart_bridge_irq(uint32_t id, uint32_t event)
{
	(void)id;
	uint8_t rc = 0;
	char const close_cmd_buf[] = "ATM2=bridge,close";

	if (event == RxIrq) {
		rc = bt_uart_bridge_getc();
		if (check_byte_num != 17) {
			switch (rc)
			{
				case 'A':
						check_byte_num = 1;
					break;
				case 'T':
					if (check_byte_num == 1)
						check_byte_num = 2;
					else
						check_byte_num = 0;
					break;
				case 'M':
					if (check_byte_num == 2)
						check_byte_num = 3;
					else
						check_byte_num = 0;
					break;
			 	case '2':
					if (check_byte_num == 3)
						check_byte_num = 4;
					else
						check_byte_num = 0;
					break;
				case '=':
					if (check_byte_num == 4)
						check_byte_num = 5;
					else
						check_byte_num = 0;
					break;
				case 'b':
					if (check_byte_num == 5)
						check_byte_num = 6;
					else
						check_byte_num = 0;
					break;
				case 'r':
					if (check_byte_num == 6)
						check_byte_num = 7;
					else
						check_byte_num = 0;
					break;
				case 'i':
					if (check_byte_num == 7)
						check_byte_num = 8;
					else
						check_byte_num = 0;
					break;
				case 'd':
					if (check_byte_num == 8)
						check_byte_num = 9;
					else
						check_byte_num = 0;
					break;
				case 'g':
					if (check_byte_num == 9)
						check_byte_num = 10;
					else
						check_byte_num = 0;
					break;
				case 'e':
					if (check_byte_num == 10)
						check_byte_num = 11;
					else if(check_byte_num == 16)
						check_byte_num = 17;
					else
						check_byte_num = 0;
					break;
				case ',':
					if (check_byte_num == 11)
						check_byte_num = 12;
					else
						check_byte_num = 0;
					break;
				case 'c':
					if (check_byte_num == 12)
						check_byte_num = 13;
					else
						check_byte_num = 0;
					break;
				case 'l':
					if (check_byte_num == 13)
						check_byte_num = 14;
					else
						check_byte_num = 0;
					break;
				case 'o':
					if (check_byte_num == 14)
						check_byte_num = 15;
					else
						check_byte_num = 0;
					break;
				case 's':
					if (check_byte_num == 15)
						check_byte_num = 16;
					else
						check_byte_num = 0;
					break;
				case 0x00: //single tone command 00
					if (check_byte_num == 23)
						check_byte_num = 25;
					else
						check_byte_num = 0;
					break;
				case 0x01: //single tone command 01
					if (check_byte_num == 23)
						check_byte_num = 24;
					else if(check_byte_num == 0)
						check_byte_num = 20;
					else
						check_byte_num = 0;
					break;
				case 0xfc: //single tone command fc
					if (check_byte_num == 21)
						check_byte_num = 22;
					else
						check_byte_num = 0;
					break;
				case 'x': //single tone command 78
					if (check_byte_num == 20)
						check_byte_num = 21;
					else
						check_byte_num = 0;
					break;
				case 0x04: //single tone command 04
					if (check_byte_num == 22)
						check_byte_num = 23;
					else
						check_byte_num = 0;
					break;
				case 'L':
					if (check_byte_num == 5)
						check_byte_num = 30;
					else
						check_byte_num = 0;
					break;
				case 'O':
					if (check_byte_num == 30)
						check_byte_num = 31;
					else
						check_byte_num = 0;
					break;
				case 'G':
					if (check_byte_num == 31)
						check_byte_num = 32;
					else
						check_byte_num = 0;
					break;
				default:
					check_byte_num = 0;
					break;
			}
		}

		if (check_byte_num == 17) {
			if (rc == KEY_ENTER) {
				log_flag = 0;
				memset(log_buf, '\0', LOG_SERVICE_BUFLEN);
				strncpy(log_buf, close_cmd_buf, strlen(close_cmd_buf));
				check_byte_num = 0;
				amebapro2_uart_bridge_open(false);
				rtw_up_sema_from_isr((_sema*)&log_rx_interrupt_sema);
			}
		} else if (check_byte_num == 32) {
			log_flag += 1;
		} else if (check_byte_num == 24) {
			btc_set_single_tone_tx(1);
		} else if (check_byte_num == 25) {
			btc_set_single_tone_tx(0);
		} else {
			amebapro2_uart_bridge_to_hci(rc);
		}
	}
}

void bt_uart_bridge_set(uint32_t baudrate, uint8_t parity)
{
	uart_baudrate = baudrate;
	uart_parity = parity;
}

void bt_uart_bridge_close(void)
{
	uart_baudrate = 0;
	uart_parity = 0;
}

void bt_uart_bridge_open(void)
{
	hal_uart_reset_rx_fifo(&log_uart);

	if (uart_baudrate != 0)
		hal_uart_set_baudrate(&log_uart, uart_baudrate);

	if (uart_parity == 1)
		hal_uart_set_format(&log_uart, 8, ParityOdd, 1);
	else if (uart_parity == 2)
		hal_uart_set_format(&log_uart, 8, ParityEven, 1);

	hal_uart_rxind_hook(&log_uart, bt_uart_bridge_irq, 0, 0);

	amebapro2_uart_bridge_open(true);
}

#else

void bt_uart_bridge_close(void)
{

}

void bt_uart_bridge_open(void)
{

}

#endif
