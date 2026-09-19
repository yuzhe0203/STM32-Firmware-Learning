#include "uart_command_parser.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char *text;
    uart_command_t command;
} uart_command_entry_t;

static bool command_matches(const char *line, const char *command){
    const size_t command_len = strlen(command);
    const size_t line_len = strlen(line);

    bool has_valid_ending = (line_len == command_len) ||
                            (line_len == command_len + 1u && line[command_len] == '\n') ||
                            (line_len == command_len + 2u && line[command_len] == '\r' && line[command_len + 1] == '\n');

    return has_valid_ending && strncmp(line, command, command_len) == 0;
}

uart_command_t uart_command_parse(const char *line){

    static const uart_command_entry_t commands[] = {
        {"LED ON", UART_COMMAND_LED_ON},
        {"LED OFF", UART_COMMAND_LED_OFF},
        {"STATUS", UART_COMMAND_STATUS},
        {"LED TOGGLE", UART_COMMAND_LED_TOGGLE},
    };

    if (line == NULL) return UART_COMMAND_INVALID;

    for (size_t i = 0u; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (command_matches(line , commands[i].text)) {
            return commands[i].command;
        }
    }
    return UART_COMMAND_INVALID;
}
