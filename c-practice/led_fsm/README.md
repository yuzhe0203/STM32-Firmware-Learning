# LED Finite State Machine Practice

A small C finite state machine that toggles an LED state in response to button presses.

## Behavior

- The LED starts in the off state.
- A new button press toggles the LED state.
- Holding the button does not repeatedly toggle the LED.
- Releasing the button does not change the LED state.
- The button must be released before another press can toggle the LED again.
- Invalid `NULL` input is handled safely.

## State Transition

```text
Initial state: LED OFF

New button press:
LED OFF -> LED ON
LED ON  -> LED OFF
```

The program detects a rising edge:

```text
button_was_pressed = false
button_pressed     = true
```

This means the button has just changed from “not pressed” to “pressed”.

## Build and Run

```bash
gcc -std=c11 -Wall -Wextra -Werror -g \
  led_fsm.c test_led_fsm.c \
  -o test_led_fsm.exe

./test_led_fsm.exe
```

## Firmware Relevance

Finite state machines are widely used in embedded firmware for:

- Button and switch handling
- LED control
- Startup and initialization sequences
- UART command parsers
- Communication protocols
- Fault and recovery handling
