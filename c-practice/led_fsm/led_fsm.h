#ifndef LED_FSM_H
#define LED_FSM_H

#include <stdbool.h>

typedef enum {
     LED_STATE_OFF = 0,
     LED_STATE_ON = 1
} led_state_t;

typedef struct {
     led_state_t state;
     bool button_was_pressed;
} led_fsm_t;

/* Initialize the controller with LED turned off. */
void led_fsm_init(led_fsm_t *fsm);

/*
 * update the FSM using the current button level.
 * 
 * The LED toggles only when button_press changes from false to true.
*/
void led_fsm_update(led_fsm_t *fsm, bool button_pressed);

/* Return true when LED is on. */
bool led_fsm_is_led_on(const led_fsm_t *fsm);

#endif
