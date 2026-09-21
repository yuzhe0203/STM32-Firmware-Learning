#ifndef UART_LED_CONTROLLER_H
#define UART_LED_CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../circular_buffer/circular_buffer.h"

#define UART_LED_CONTROLLER_LINE_CAPACITY 32u

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
} led_state_t;

typedef struct {
    circular_buffer_t rx_buffer;
    char line[UART_LED_CONTROLLER_LINE_CAPACITY];
    size_t line_len;
    bool discarding_line;
    led_state_t led_state;
} uart_led_controller_t;

/* Initialize the controller with the LED off. */
void uart_led_controller_init(uart_led_controller_t *controller);

/*
 * Receive one byte from UART.
 *
 * Bytes are buffered, then processed into a command line when '\n' arrives.
 * Returns false only for invalid input or if the circular buffer is full.
 */
bool uart_led_controller_receive_byte(uart_led_controller_t *controller,
                                      uint8_t byte);

/* Return true when the simulated LED is on. */
bool uart_led_controller_is_led_on(const uart_led_controller_t *controller);

#endif
