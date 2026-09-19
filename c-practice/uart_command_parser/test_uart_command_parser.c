#include "uart_command_parser.h"

#include <assert.h>
#include <stdio.h>

static void test_led_on_command(void)
{
    assert(uart_command_parse("LED ON") == UART_COMMAND_LED_ON);
    assert(uart_command_parse("LED ON\n") == UART_COMMAND_LED_ON);
    assert(uart_command_parse("LED ON\r\n") == UART_COMMAND_LED_ON);

    printf("test_led_on_command passed.\n");
}

static void test_led_off_command(void)
{
    assert(uart_command_parse("LED OFF") == UART_COMMAND_LED_OFF);
    assert(uart_command_parse("LED OFF\n") == UART_COMMAND_LED_OFF);
    assert(uart_command_parse("LED OFF\r\n") == UART_COMMAND_LED_OFF);

    printf("test_led_off_command passed.\n");
}

static void test_led_toggle_command(void)
{
    assert(uart_command_parse("LED TOGGLE") == UART_COMMAND_LED_TOGGLE);
    assert(uart_command_parse("LED TOGGLE\n") == UART_COMMAND_LED_TOGGLE);
    assert(uart_command_parse("LED TOGGLE\r\n") == UART_COMMAND_LED_TOGGLE);

    printf("test_led_toggle_command passed.\n");
}

static void test_status_command(void)
{
    assert(uart_command_parse("STATUS") == UART_COMMAND_STATUS);
    assert(uart_command_parse("STATUS\n") == UART_COMMAND_STATUS);
    assert(uart_command_parse("STATUS\r\n") == UART_COMMAND_STATUS);

    printf("test_status_command passed.\n");
}

static void test_invalid_commands(void)
{
    assert(uart_command_parse(NULL) == UART_COMMAND_INVALID);
    assert(uart_command_parse("") == UART_COMMAND_INVALID);
    assert(uart_command_parse("LED") == UART_COMMAND_INVALID);
    assert(uart_command_parse("LEDON") == UART_COMMAND_INVALID);
    assert(uart_command_parse("LED on") == UART_COMMAND_INVALID);
    assert(uart_command_parse("STATUS NOW") == UART_COMMAND_INVALID);
    assert(uart_command_parse("HELLO") == UART_COMMAND_INVALID);

    printf("test_invalid_commands passed.\n");
}

int main(void)
{
    test_led_on_command();
    test_led_off_command();
    test_led_toggle_command();
    test_status_command();
    test_invalid_commands();

    printf("All UART command parser tests passed.\n");
    return 0;
}
