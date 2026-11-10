#include <main.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "app_watchdog.h"

static IWDG_HandleTypeDef *s_hiwdg = NULL;
static volatile uint32_t s_last_tick[APP_WDG_ID_COUNT];

/* Public Part */
void app_watchdog_init(IWDG_HandleTypeDef *hiwdg) {
	s_hiwdg = hiwdg;

	if (s_hiwdg == NULL) {
		Error_Handler();
	}
}

void app_watchdog_task(void) {
	uint32_t next_wake = osKernelGetTickCount();

	for (uint32_t i = 0u; i < APP_WDG_ID_COUNT; i++) {
		s_last_tick[i] = next_wake;
	}

	for (;;) {
		uint32_t now = osKernelGetTickCount();
		bool all_ok = true;

		for (uint32_t i = 0u; i < APP_WDG_ID_COUNT; i++) {
			if ((now - s_last_tick[i]) > APP_WATCHDOG_TASK_LIMIT_MS) {
				all_ok = false;
			}
		}

		if (all_ok) {
			HAL_IWDG_Refresh(s_hiwdg);
		}

		next_wake += APP_WATCHDOG_KICK_PERIOD_MS;
		osDelayUntil(next_wake);
	}
}

void app_watchdog_checkin(app_watchdog_id_t id) {
	if (id < APP_WDG_ID_COUNT) {
		s_last_tick[id] = osKernelGetTickCount();
	}
}

/* Private Part */

/* System Configuration */
