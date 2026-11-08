#include "app_led.h"
#include <main.h>
#include <stdbool.h>
#include "cmsis_os.h"

typedef struct {
	app_led_mode_t mode;
	uint32_t period;
}led_request_t;

bool ledStatus;
static osMessageQueueId_t LEDQueue = NULL;
static volatile app_led_mode_t current_mode = APP_LED_OFF;
static volatile uint32_t current_ms = 500u;

static void write_leds(bool a_on, bool b_on);

/* Public Part */
void app_led_init(void) {
	LEDQueue = osMessageQueueNew(4, sizeof(led_request_t), NULL);

	if (LEDQueue == NULL) {
		Error_Handler();
	}
}

bool app_led_request(app_led_mode_t mode, uint32_t period_ms) {
	led_request_t req;

	if (mode > APP_LED_HOLD) {
		return false;
	}

	if (mode == APP_LED_BLINK) {
		if ((period_ms > BLINK_MAX_MS) || (period_ms < BLINK_MIN_MS)) {
			return false;
		}
	}

	if (LEDQueue == NULL) {
		return false;
	}

	req.mode = mode;
	req.period = period_ms;

	if (osMessageQueuePut(LEDQueue, &req, 0, 0) != osOK) {
		return false;
	}

	return true;
}

void app_led_task(void) {
	app_led_mode_t mode = APP_LED_OFF;
	uint32_t period_ms = 0u;
	bool phase = false;
	led_request_t req;

	uint32_t timeout;
	osStatus_t status;

	for(;;) {
		timeout = (mode == APP_LED_BLINK)? period_ms : osWaitForever;
		status = osMessageQueueGet(LEDQueue, &req, NULL, timeout);

		if (status == osOK) {
			mode = req.mode;

			switch(mode) {

			case APP_LED_ON:
				write_leds(true, true);
				break;

			case APP_LED_OFF:
				write_leds(false, false);
				break;

			case APP_LED_BLINK:
				period_ms = req.period;
				phase = true;
				write_leds(phase, !phase);
				break;

			case APP_LED_HOLD:
				break;

			default:
				break;
			}

			current_mode = mode;
			current_ms = period_ms;
		}
		else if ((status == osErrorTimeout) && (mode == APP_LED_BLINK)) {
			phase = !phase;
			write_leds(phase, !phase);
		}
	}
}

app_led_mode_t app_led_get_mode(void) {
	return current_mode;
}

uint32_t app_led_get_period(void) {
	return current_ms;
}

/* Private Part */
static void write_leds(bool a_on, bool b_on) {
	uint32_t ledMask_high = 0u;
	uint32_t ledMask_low = 0u;

	if (a_on) {
		ledMask_high |= (uint32_t)LED_BLINK_A_Pin;
	}
	else {
		ledMask_low |= (uint32_t)LED_BLINK_A_Pin;
	}

	if (b_on) {
		ledMask_high |= (uint32_t)LED_BLINK_B_Pin;
	}
	else {
		ledMask_low |= (uint32_t)LED_BLINK_B_Pin;
	}

	LED_BLINK_A_GPIO_Port->BSRR = ledMask_high | (ledMask_low << 16);
}

/* System Configuration */
