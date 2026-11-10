#ifndef APP_WATCHDOG_H
#define APP_WATCHDOG_H

#include <main.h>

typedef enum {
	APP_WDG_ID_DEFAULT,
	APP_WDG_ID_LED,
	APP_WDG_ID_SENSOR,
	APP_WDG_ID_MCP9808,
	APP_WDG_ID_COUNT,
} app_watchdog_id_t;

#define APP_WATCHDOG_KICK_PERIOD_MS 500u
#define APP_WATCHDOG_TASK_LIMIT_MS 3000u

void app_watchdog_init(IWDG_HandleTypeDef *hiwdg);

void app_watchdog_task(void);

void app_watchdog_checkin(app_watchdog_id_t id);

#endif
