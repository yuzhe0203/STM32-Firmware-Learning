#ifdef APP_CMD_H
#define APP_CMD_H
#endif

#include <main.h>

void app_cmd_process_byte(uint8_t byte);

void app_cmd_on_rx_overflow(void);
