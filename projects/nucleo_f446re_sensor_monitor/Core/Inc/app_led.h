#ifndef APP_LED_H
#define APP_LED_H

#include <main.h>
#include <stdbool.h>
#include "cmsis_os.h"

typedef enum {
	APP_LED_ON,
	APP_LED_OFF,
	APP_LED_BLINK,
	APP_LED_HOLD,
}app_led_mode_t;

#define BLINK_MIN_MS 100u
#define BLINK_MAX_MS 2000u

void app_led_init(void);

void app_led_task(void);

bool app_led_request(app_led_mode_t mode, uint32_t period_ms);

app_led_mode_t app_led_get_mode(void);

uint32_t app_led_get_period(void);

#endif
