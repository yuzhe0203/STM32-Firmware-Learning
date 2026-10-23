#ifdef APP_UART_H
#define APP_UART_H
#endif

#include <main.h>
#include <stdbool.h>
#include "cmsis_os.h"

typedef struct {
	uint32_t rx_events;
	uint32_t rx_dropped_bytes;
	uint32_t tx_dropped_messages;
}app_uart_stats_t;

void app_uart_init(UART_HandleTypeDef *huart);

void app_uart_send_text(const char *text);

void app_uart_start_rx(osMessageQueueId_t queue);

bool app_uart_read_byte(uint8_t *byte);

bool app_uart_take_rx_overflow(void);

void app_uart_get_stats(app_uart_stats_t *out);

void app_uart_reset_stats(void);
