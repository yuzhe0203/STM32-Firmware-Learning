#include "led_fsm.h"
#include <stddef.h>

void led_fsm_init(led_fsm_t *fsm){
    if (fsm == NULL) return;

    fsm->state = LED_STATE_OFF;
    fsm->button_was_pressed = false;
}

void led_fsm_update(led_fsm_t *fsm, bool button_pressed){
    if (fsm == NULL) return;

    if (button_pressed && !fsm->button_was_pressed)
        fsm->state = (fsm->state == LED_STATE_OFF) ? LED_STATE_ON : LED_STATE_OFF;

    fsm->button_was_pressed = button_pressed;
}

bool led_fsm_is_led_on(const led_fsm_t *fsm){
    return (fsm != NULL && fsm->state == LED_STATE_ON);
}