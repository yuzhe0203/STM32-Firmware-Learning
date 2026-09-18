#include "led_fsm.h"

#include <assert.h>
#include <stdio.h>

static void test_initial_state_is_off(void)
{
    led_fsm_t fsm;

    led_fsm_init(&fsm);

    assert(!led_fsm_is_led_on(&fsm));

    printf("test_initial_state_is_off passed.\n");
}

static void test_single_press_turns_led_on(void)
{
    led_fsm_t fsm;

    led_fsm_init(&fsm);

    led_fsm_update(&fsm, true);

    assert(led_fsm_is_led_on(&fsm));

    printf("test_single_press_turns_led_on passed.\n");
}

static void test_second_press_turns_led_off(void)
{
    led_fsm_t fsm;

    led_fsm_init(&fsm);

    led_fsm_update(&fsm, true);
    led_fsm_update(&fsm, false);
    led_fsm_update(&fsm, true);

    assert(!led_fsm_is_led_on(&fsm));

    printf("test_second_press_turns_led_off passed.\n");
}

static void test_holding_button_does_not_toggle_repeatedly(void)
{
    led_fsm_t fsm;

    led_fsm_init(&fsm);

    led_fsm_update(&fsm, true);
    assert(led_fsm_is_led_on(&fsm));

    led_fsm_update(&fsm, true);
    led_fsm_update(&fsm, true);
    led_fsm_update(&fsm, true);

    assert(led_fsm_is_led_on(&fsm));

    printf("test_holding_button_does_not_toggle_repeatedly passed.\n");
}

static void test_releasing_button_does_not_change_led(void)
{
    led_fsm_t fsm;

    led_fsm_init(&fsm);

    led_fsm_update(&fsm, true);
    assert(led_fsm_is_led_on(&fsm));

    led_fsm_update(&fsm, false);

    assert(led_fsm_is_led_on(&fsm));

    printf("test_releasing_button_does_not_change_led passed.\n");
}

static void test_null_input_is_safe(void)
{
    led_fsm_t fsm;

    led_fsm_init(&fsm);

    led_fsm_init(NULL);
    led_fsm_update(NULL, true);

    assert(!led_fsm_is_led_on(NULL));

    printf("test_null_input_is_safe passed.\n");
}

int main(void)
{
    test_initial_state_is_off();
    test_single_press_turns_led_on();
    test_second_press_turns_led_off();
    test_holding_button_does_not_toggle_repeatedly();
    test_releasing_button_does_not_change_led();
    test_null_input_is_safe();

    printf("All LED FSM tests passed.\n");
    return 0;
}
