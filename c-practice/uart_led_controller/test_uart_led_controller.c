#include "uart_led_controller.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

static void send_uart_text(uart_led_controller_t *controller, const char *text)
{
    size_t index = 0u;

    while (text[index] != '\0')
    {
        assert(uart_led_controller_receive_byte(controller,
                                                (uint8_t)text[index]));
        index++;
    }
}

static void test_initial_state_is_off(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    assert(!uart_led_controller_is_led_on(&controller));

    printf("test_initial_state_is_off passed.\n");
}

static void test_led_on_command(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    send_uart_text(&controller, "LED ON\r\n");

    assert(uart_led_controller_is_led_on(&controller));

    printf("test_led_on_command passed.\n");
}

static void test_led_off_command(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    send_uart_text(&controller, "LED ON\n");
    assert(uart_led_controller_is_led_on(&controller));

    send_uart_text(&controller, "LED OFF\n");
    assert(!uart_led_controller_is_led_on(&controller));

    printf("test_led_off_command passed.\n");
}

static void test_toggle_command(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    send_uart_text(&controller, "LED TOGGLE\n");
    assert(uart_led_controller_is_led_on(&controller));

    send_uart_text(&controller, "LED TOGGLE\n");
    assert(!uart_led_controller_is_led_on(&controller));

    printf("test_toggle_command passed.\n");
}

static void test_status_does_not_change_led(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    send_uart_text(&controller, "STATUS\n");
    assert(!uart_led_controller_is_led_on(&controller));

    send_uart_text(&controller, "LED ON\n");
    send_uart_text(&controller, "STATUS\n");
    assert(uart_led_controller_is_led_on(&controller));

    printf("test_status_does_not_change_led passed.\n");
}

static void test_invalid_command_does_not_change_led(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    send_uart_text(&controller, "HELLO\n");
    assert(!uart_led_controller_is_led_on(&controller));

    send_uart_text(&controller, "LED ON\n");
    assert(uart_led_controller_is_led_on(&controller));

    send_uart_text(&controller, "LED MAYBE\n");
    assert(uart_led_controller_is_led_on(&controller));

    printf("test_invalid_command_does_not_change_led passed.\n");
}

static void test_incomplete_command_does_not_execute(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    send_uart_text(&controller, "LED ON");

    assert(!uart_led_controller_is_led_on(&controller));

    send_uart_text(&controller, "\n");

    assert(uart_led_controller_is_led_on(&controller));

    printf("test_incomplete_command_does_not_execute passed.\n");
}

static void test_long_line_is_discarded(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    send_uart_text(&controller,
                   "THIS COMMAND IS FAR TOO LONG TO FIT IN THE INPUT LINE\n");

    assert(!uart_led_controller_is_led_on(&controller));

    send_uart_text(&controller, "LED ON\n");

    assert(uart_led_controller_is_led_on(&controller));

    printf("test_long_line_is_discarded passed.\n");
}

static void test_null_input_is_safe(void)
{
    uart_led_controller_t controller;

    uart_led_controller_init(&controller);

    assert(!uart_led_controller_receive_byte(NULL, (uint8_t)'L'));
    assert(!uart_led_controller_is_led_on(NULL));

    printf("test_null_input_is_safe passed.\n");
}

int main(void)
{
    test_initial_state_is_off();
    test_led_on_command();
    test_led_off_command();
    test_toggle_command();
    test_status_does_not_change_led();
    test_invalid_command_does_not_change_led();
    test_incomplete_command_does_not_execute();
    test_long_line_is_discarded();
    test_null_input_is_safe();

    printf("All UART LED controller tests passed.\n");
    return 0;
}
