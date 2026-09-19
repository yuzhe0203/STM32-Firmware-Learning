#ifndef UART_COMMAND_PARSER_H
#define UART_COMMAND_PARSER_H

typedef enum {
    UART_COMMAND_INVALID = 0,
    UART_COMMAND_LED_ON,
    UART_COMMAND_LED_OFF,
    UART_COMMAND_LED_TOGGLE,
    UART_COMMAND_STATUS,
} uart_command_t;

/*
 * Parser one null-terminated UART command line.
 * 
 * Accepted commands:
 *  - LED ON
 *  - LED OFF
 *  - LED TOGGLE
 *  - STATUS
 *
 * A trailing '\n' or '\r\n' is accepted.
 * Invalid input returns UART_COMMAND_INVALID.
*/
uart_command_t uart_command_parse(const char *line);

#endif
