/*
 * Core1 entry point wrapper
 */

#include "main_core1.h"

#include "output.h"
#include "pwm/pwm.h"
#include "trigger.h"
#include "pico.h"

void abort_all(void) {
    pwm_abort();
    trigger_abort();
}

void reset_to_defaults_all(void) {
    output_init();

    abort_all();

    trigger_init();
    pwm_init_module();
}

void main_core1_entry(void) {
    // Initialize trigger manager, PWM configuration, and output state
    reset_to_defaults_all();

    while (true) {
        tight_loop_contents();
    }
}

