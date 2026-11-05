#include <main.h>
#include "app_reset.h"

static app_reset_cause_t s_cause = APP_RESET_UNKNOWN;

/* Public Part */
void app_reset_capture(void) {
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
		s_cause = APP_RESET_IWDG;
	}
	else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
		s_cause = APP_RESET_WWDG;
	}
	else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
		s_cause = APP_RESET_SOFTWARE;
	}
	else if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST)) {
		s_cause = APP_RESET_LOW_POWER;
	}
	else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) || __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
		s_cause = APP_RESET_POWER_ON;
	}
	else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
		s_cause = APP_RESET_PIN;
	}
	else {
		s_cause = APP_RESET_UNKNOWN;
	}

	__HAL_RCC_CLEAR_RESET_FLAGS();
}

app_reset_cause_t app_reset_get_cause(void) {
	return s_cause;
}

const char *app_reset_cause_to_string(app_reset_cause_t cause) {
	switch(cause) {

	case APP_RESET_IWDG:
		return "IWDG";
		break;

	case APP_RESET_WWDG:
		return "WWDG";
		break;

	case APP_RESET_SOFTWARE:
		return "Software";
		break;

	case APP_RESET_LOW_POWER:
		return "Low Power";
		break;

	case APP_RESET_POWER_ON:
		return "Power On / Brown Out";
		break;

	case APP_RESET_PIN:
		return "Reset Pin";
		break;

	default:
		return "Unknown";
		break;

	}
}

/* Private Part */

/* System Configuration */
