#include "uart_led_controller.h"

#include "../uart_command_parser/uart_command_parser.h"

static void uart_led_controller_apply_command(uart_led_controller_t *controller,
                                              uart_command_t command)
{
    if (command == UART_COMMAND_LED_ON)
    {
        controller->led_state = LED_STATE_ON;
    }
    else if (command == UART_COMMAND_LED_OFF)
    {
        controller->led_state = LED_STATE_OFF;
    }
    else if (command == UART_COMMAND_LED_TOGGLE)
    {
        if (controller->led_state == LED_STATE_OFF)
        {
            controller->led_state = LED_STATE_ON;
        }
        else
        {
            controller->led_state = LED_STATE_OFF;
        }
    }
    else
    {
        /*
         * STATUS and INVALID do not change the LED state.
         *
         * On a real STM32 project, STATUS could cause UART transmit code
         * to send back "LED ON\r\n" or "LED OFF\r\n".
         */
    }
}

static void uart_led_controller_process_byte(uart_led_controller_t *controller,
                                             uint8_t byte)
{
    uart_command_t command;

    if (byte == (uint8_t)'\r')
    {
        return;
    }

    if (byte == (uint8_t)'\n')
    {
        if (!controller->discarding_line)
        {
            controller->line[controller->line_len] = '\0';

            command = uart_command_parse(controller->line);
            uart_led_controller_apply_command(controller, command);
        }

        controller->line_len = 0u;
        controller->discarding_line = false;
        return;
    }

    if (controller->discarding_line)
    {
        return;
    }

    if (controller->line_len >= UART_LED_CONTROLLER_LINE_CAPACITY - 1u)
    {
        controller->line_len = 0u;
        controller->discarding_line = true;
        return;
    }

    controller->line[controller->line_len] = (char)byte;
    controller->line_len++;
}

void uart_led_controller_init(uart_led_controller_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    circular_buffer_init(&controller->rx_buffer);

    controller->line_len = 0u;
    controller->discarding_line = false;
    controller->led_state = LED_STATE_OFF;
}

bool uart_led_controller_receive_byte(uart_led_controller_t *controller,
                                      uint8_t byte)
{
    uint8_t received_byte = 0u;

    if (controller == NULL)
    {
        return false;
    }

    if (!circular_buffer_push(&controller->rx_buffer, byte))
    {
        return false;
    }

    while (circular_buffer_pop(&controller->rx_buffer, &received_byte))
    {
        uart_led_controller_process_byte(controller, received_byte);
    }

    return true;
}

bool uart_led_controller_is_led_on(const uart_led_controller_t *controller)
{
    return controller != NULL && controller->led_state == LED_STATE_ON;
}
