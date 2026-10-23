#ifdef APP_CMD_H
#define APP_CMD_H
#endif

#include <main.h>

#define BLINK_MIN_MS 100u
#define BLINK_MAX_MS 2000u

void app_cmd_process_byte(uint8_t byte);

void app_cmd_on_rx_overflow(void);
