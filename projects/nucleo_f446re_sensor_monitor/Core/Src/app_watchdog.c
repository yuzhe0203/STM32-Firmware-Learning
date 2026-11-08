#include <main.h>
#include "cmsis_os.h"
#include "app_watchdog.h"

static IWDG_HandleTypeDef *s_hiwdg = NULL;

/* Public Part */
void app_watchdog_init(IWDG_HandleTypeDef *hiwdg) {
	s_hiwdg = hiwdg;

	if (s_hiwdg == NULL) {
		Error_Handler();
	}
}

void app_watchdog_task(void) {
	uint32_t next_wake = osKernelGetTickCount();

	for (;;) {
		HAL_IWDG_Refresh(s_hiwdg);
		next_wake += APP_WATCHDOG_KICK_PERIOD_MS;
		osDelayUntil(next_wake);
	}
}

/* Private Part */

/* System Configuration */
