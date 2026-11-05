#ifndef APP_RESET_H
#define APP_RESET_H

typedef enum {
	APP_RESET_UNKNOWN,
	APP_RESET_IWDG,
	APP_RESET_WWDG,
	APP_RESET_SOFTWARE,
	APP_RESET_LOW_POWER,
	APP_RESET_POWER_ON,
	APP_RESET_PIN,
} app_reset_cause_t;

void app_reset_capture(void);

app_reset_cause_t app_reset_get_cause(void);

const char *app_reset_cause_to_string(app_reset_cause_t cause);

#endif
