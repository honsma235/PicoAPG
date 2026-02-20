/*
 * Central output state control
 */

#include "pwm/pwm_gpio.h"
#include "output.h"

output_state_t g_output_state;

void output_init(void) {
    g_output_state.enabled = false;
    output_apply_state();
}

void output_apply_state(void) {
    /* Centralize the disable path; enabling remains with each submodule. */
    if (g_output_state.enabled) {
        pwm_outputs_attach();
    } else {
        pwm_outputs_detach();
    }
}