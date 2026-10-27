#include "app_cmd.h"
#include "app_uart.h"
#include "app_led.h"
#include "app_sensor.h"
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
		app_uart_send_text(" sample <100-10000>\r\n> ");
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

		switch(mode) {

		case APP_LED_ON:
			(void)snprintf(res, sizeof(res), "STATUS: LED ON.\r\n> ");
			break;

		case APP_LED_OFF:
			(void)snprintf(res, sizeof(res), "STATUS: LED OFF.\r\n> ");
			break;

		case APP_LED_BLINK:
			(void)snprintf(res, sizeof(res),
					       "STATUS: BLINK %lu ms.\r\n> ",
						   (unsigned long)period);
			break;

		case APP_LED_HOLD:
			(void)snprintf(res, sizeof(res), "STATUS: BLINK STOPPED.\r\n> ");
			break;

		default:
			(void)snprintf(res, sizeof(res), "STATUS: UNKOWN.\r\n> ");
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

/* System Configuration */
