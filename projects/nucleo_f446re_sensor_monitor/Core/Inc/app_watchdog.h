#ifndef APP_WATCHDOG_H
#define APP_WATCHDOG_H

#include <main.h>

#define APP_WATCHDOG_KICK_PERIOD_MS 500u

void app_watchdog_init(IWDG_HandleTypeDef *hiwdg);

void app_watchdog_task(void);

#endif
