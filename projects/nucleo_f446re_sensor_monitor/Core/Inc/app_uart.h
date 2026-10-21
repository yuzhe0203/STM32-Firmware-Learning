#ifdef APP_UART_H
#define APP_UART_H
#endif

#include <main.h>

void app_uart_init(UART_HandleTypeDef *huart);

void app_uart_send_text(const char *text);
