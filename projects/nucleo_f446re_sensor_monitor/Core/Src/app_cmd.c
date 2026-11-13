#include "app_cmd.h"
#include "app_uart.h"
#include "app_led.h"
#include "app_sensor.h"
#include "app_config.h"
#include "app_mcp9808.h"
#include "temp_zone.h"
#include "app_reset.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define COMMAND_CAPACITY 32u

static char command[COMMAND_CAPACITY];
static uint16_t command_len = 0u;
static uint8_t discard_command = 0u;
static volatile uint32_t rx_command_overflow = 0u;
static uint8_t last_was_cr = 0u;

static void process_command(const char *command);
static void send_adc(void);
static void split_temp_x10(int32_t value, const char **sign, uint32_t *int_part, uint32_t *frac_part);
static void send_config(void);
static const char *mode_to_string(app_led_mode_t mode);
static void send_temp(void);

/* Public Part */
void app_cmd_process_byte(uint8_t byte) {
	uint8_t last_cr = last_was_cr;

	last_was_cr = (byte == (uint8_t)'\r')? 1u : 0u;

	if ((byte == (uint8_t)'\n') && last_cr != 0u) {
		return;
	}

	if ((byte == (uint8_t)'\n') || (byte == (uint8_t)'\r')) {

		if (discard_command != 0u) {
			discard_command = 0u;
			command_len = 0u;
			app_uart_send_text("ERROR: command too long.\r\n> ");
			return;
		}

		command[command_len] = '\0';
		process_command(command);
		command_len = 0u;
		return;
	}

	if (discard_command != 0u) {
		return;
	}

	if (command_len < (COMMAND_CAPACITY - 1u)) {
		command[command_len] = (char)byte;
		command_len++;
	}
	else {
		discard_command = 1u;
		rx_command_overflow++;
	}

}

void app_cmd_on_rx_overflow(void) {
	command_len = 0u;
	discard_command = 0u;
	app_uart_send_text("ERROR: receive buffer overflow.\r\n> ");
}

/* Private Part */
static void process_command(const char *command) {
	char *end_ptr;
	unsigned long requested_period;

	if (strcmp(command, "help") == 0) {
		app_uart_send_text("Commands:\r\n");
		app_uart_send_text(" led on\r\n");
		app_uart_send_text(" led off\r\n");
		app_uart_send_text(" blink <100-2000>\r\n");
		app_uart_send_text(" status\r\n");
		app_uart_send_text(" stats\r\n");
		app_uart_send_text(" reset stats\r\n");
		app_uart_send_text(" adc\r\n");
		app_uart_send_text(" sample <100-10000>\r\n");
		app_uart_send_text(" config show\r\n");
		app_uart_send_text(" config save\r\n");
		app_uart_send_text(" config reset\r\n");
		app_uart_send_text(" temp\r\n");
#ifdef APP_DEBUG_COMMANDS
		app_uart_send_text(" wdg spin\r\n");
		app_uart_send_text(" wdg block\r\n> ");
#endif
	}
	else if (strcmp(command, "led on") == 0) {
		app_led_request(APP_LED_ON, 0);
		app_uart_send_text("DONE: LED is ON.\r\n> ");
	}
	else if (strcmp(command, "led off") == 0) {
		app_led_request(APP_LED_OFF, 0);
		app_uart_send_text("DONE: LED is OFF.\r\n> ");
	}
	else if (strcmp(command, "blink off") == 0) {
		app_led_request(APP_LED_HOLD, 0);
		app_uart_send_text("DONE: Blink is OFF.\r\n> ");
	}
	else if (strncmp(command, "blink ", 6u) == 0) {
		requested_period = strtoul(&command[6], &end_ptr, 10);

		if ((*end_ptr != '\0') || (requested_period < BLINK_MIN_MS) || (requested_period > BLINK_MAX_MS)) {
			app_uart_send_text("ERROR: use 'blink <100-2000>' or 'blink off'.\r\n> ");
			return;
		}

		if (app_led_request(APP_LED_BLINK, (uint32_t)requested_period)) {
			app_uart_send_text("DONE: LED Blink.\r\n> ");
		}
		else {
			app_uart_send_text("ERROR: LED busy.\r\n> ");
		}
	}
	else if (strcmp(command, "stats") == 0) {
		char res[192];
		app_uart_stats_t stats;
		uint32_t uptime = HAL_GetTick();
		app_uart_get_stats(&stats);

		(void)snprintf(res, sizeof(res),
				       "STATS: Uptime= %lu.%lu(s), RX Events= %lu, RX Dropped bytes= %lu, TX Dropped Messages= %lu, RX Command Overflow= %lu.\r\n> ",
					   (unsigned long)uptime / 1000u,
					   (unsigned long)uptime % 1000u,
					   (unsigned long)stats.rx_events,
					   (unsigned long)stats.rx_dropped_bytes,
					   (unsigned long)stats.tx_dropped_messages,
					   (unsigned long)rx_command_overflow);

		app_uart_send_text(res);
	}
	else if (strcmp(command, "reset stats") == 0) {
		app_uart_reset_stats();
		rx_command_overflow = 0u;
		app_uart_send_text("DONE: Stats Reset.\r\n> ");
	}
	else if(strcmp(command, "status") == 0) {
		char res[64];
		app_led_mode_t mode = app_led_get_mode();
		uint32_t period = app_led_get_period();
		app_reset_cause_t cause = app_reset_get_cause();
		const char *cause_str = app_reset_cause_to_string(cause);

		switch(mode) {

		case APP_LED_ON:
			(void)snprintf(res, sizeof(res), "STATUS: LED ON, Reset= %s.\r\n> ", cause_str);
			break;

		case APP_LED_OFF:
			(void)snprintf(res, sizeof(res), "STATUS: LED OFF, Reset= %s.\r\n> ", cause_str);
			break;

		case APP_LED_BLINK:
			(void)snprintf(res, sizeof(res),
					       "STATUS: BLINK %lu ms, Reset= %s.\r\n> ",
						   (unsigned long)period,
						   cause_str);
			break;

		case APP_LED_HOLD:
			(void)snprintf(res, sizeof(res), "STATUS: BLINK STOPPED, Reset= %s.\r\n> ", cause_str);
			break;

		default:
			(void)snprintf(res, sizeof(res), "STATUS: UNKNOWN, Reset= %s.\r\n> ", cause_str);
			break;
		}

		app_uart_send_text(res);
	}
	else if (strcmp(command, "adc") == 0) {
		send_adc();
	}
	else if (strncmp(command, "sample ", 6u) == 0) {
		requested_period = strtoul(&command[6], &end_ptr, 10);

		if ((*end_ptr != '\0') || !app_sensor_set_period(requested_period)) {
			app_uart_send_text("ERROR: use 'sample <100-10000>'\r\n> ");
		}
		else {
			app_uart_send_text("DONE: Sample Applied.\r\n> ");
		}
	}
	else if (strcmp(command, "config show") == 0) {
		send_config();
	}
	else if (strcmp(command, "config save") == 0) {
		app_config_values_t values = {0};
		values.blink_ms = app_led_get_period();
		values.led_mode = app_led_get_mode();
		values.sample_ms = app_sensor_get_period();

		if (app_config_save(&values)) {
			app_uart_send_text("DONE: Config Save Succeed.\r\n> ");
		}
		else {
			app_uart_send_text("ERROR: Config Save Failed.\r\n> ");
		}
	}
	else if (strcmp(command, "config reset") == 0) {
		if (app_config_erase()) {
			app_led_request(APP_LED_OFF, 0);
			app_sensor_set_period(SAMPLE_DEFAULT_PERIOD_MS);
			app_uart_send_text("DONE: Config Reset Succeed.\r\n> ");
		}
		else {
			app_uart_send_text("ERROR: Config Reset Failed.\r\n> ");
		}
	}
	else if (strcmp(command, "temp") == 0) {
		send_temp();
	}
#ifdef APP_DEBUG_COMMANDS
	else if (strcmp(command, "wdg spin") == 0) {
		app_uart_send_text("WDG TEST: spin.\r\n");
		for (;;) {
			//empty loop
		}
	}
	else if (strcmp(command, "wdg block") == 0) {
		app_uart_send_text("WDG TEST: block.\r\n");
		osThreadSuspend(osThreadGetId());
	}
#endif
	else {
		app_uart_send_text("ERROR: unknown command, type 'help'.\r\n> ");
	}
}

static void send_adc(void) {
	char res[160];
	app_sensor_snapshot_t snap;
	const char *temp_sign;
	uint32_t temp_int;
	uint32_t temp_frac;
	const char *avg_sign;
	uint32_t avg_int;
	uint32_t avg_frac;

	app_sensor_read(&snap);

	split_temp_x10(snap.temp_x10, &temp_sign, &temp_int, &temp_frac);
	split_temp_x10(snap.avg_x10, &avg_sign, &avg_int, &avg_frac);

	(void)snprintf(res, sizeof(res),
				   "ADC: raw=%lu, temp=%s%lu.%lu C, avg=%s%lu.%lu C, samples=%lu, processed=%lu, timeouts=%lu\r\n> ",
				   (unsigned long)snap.raw,
				   temp_sign,
				   (unsigned long)temp_int,
				   (unsigned long)temp_frac,
				   avg_sign,
				   (unsigned long)avg_int,
				   (unsigned long)avg_frac,
				   (unsigned long)snap.samples,
				   (unsigned long)snap.processed,
				   (unsigned long)snap.timeouts);

	app_uart_send_text(res);

}

static void split_temp_x10(int32_t value, const char **sign, uint32_t *int_part, uint32_t *frac_part) {
	uint32_t abs_value;

	if (value < 0) {
		*sign = "-";
		abs_value = -value;
	}
	else {
		*sign = "";
		abs_value = value;
	}

	*int_part = abs_value / 10;
	*frac_part = abs_value % 10;
}

static const char *mode_to_string(app_led_mode_t mode) {
	switch(mode) {

	case APP_LED_ON:
		return "ON";
		break;

	case APP_LED_OFF:
		return "OFF";
		break;

	case APP_LED_BLINK:
		return "BLINK";
		break;

	case APP_LED_HOLD:
		return "STOPPED";
		break;

	default:
		return "UNKNOWN";
		break;

	}
}

static void send_config(void) {
	char res[160];
	app_config_values_t values = {0};

	if (app_config_read(&values)) {
		(void)snprintf(res, sizeof(res),
				       "CONFIG NOW: Mode: %s, Blink= %lu ms, Sample= %lu ms\r\nCONFIG FLASH: Mode: %s, Blink= %lu ms, Sample= %lu ms\r\n> ",
					   mode_to_string(app_led_get_mode()),
					   (unsigned long)app_led_get_period(),
					   (unsigned long)app_sensor_get_period(),
					   mode_to_string(values.led_mode),
					   (unsigned long)values.blink_ms,
					   (unsigned long)values.sample_ms);
	}
	else {
		(void)snprintf(res, sizeof(res),
					   "CONFIG NOW: Mode: %s, Blink= %lu ms, Sample= %lu ms\r\nCONFIG FLASH: None\r\n> ",
					   mode_to_string(app_led_get_mode()),
					   (unsigned long)app_led_get_period(),
					   (unsigned long)app_sensor_get_period());
	}

	app_uart_send_text(res);
}

static void send_temp(void) {
	char res[128];
	app_sensor_snapshot_t chip;
	const char *chip_sign;
	uint32_t chip_int;
	uint32_t chip_frac;
	app_mcp9808_snapshot_t temp;
	const char *temp_sign;
	uint32_t temp_int;
	uint32_t temp_frac;

	app_sensor_read(&chip);
	app_mcp9808_read(&temp);

	split_temp_x10(chip.avg_x10, &chip_sign, &chip_int, &chip_frac);
	split_temp_x10(temp.temp_x10, &temp_sign, &temp_int, &temp_frac);

	(void)snprintf(res, sizeof(res),
			 	   "TEMP: Chip= %s%lu.%lu C, Ambient= %s%lu.%lu C, Zone= %s, OK= %lu, ERROR= %lu\r\n> ",
				   chip_sign,
				   (unsigned long)chip_int,
				   (unsigned long)chip_frac,
				   temp_sign,
				   (unsigned long)temp_int,
				   (unsigned long)temp_frac,
				   temp_zone_to_string(temp.zone),
				   (unsigned long)temp.ok_count,
				   (unsigned long)temp.error_count);

	app_uart_send_text(res);
}

/* System Configuration */
