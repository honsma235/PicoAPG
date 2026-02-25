/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 *
 * Core1 entry point wrapper
 */

#include "pico.h"

#include "apg/apg.h"
#include "pwm/pwm.h"

#include "main_core1.h"
#include "output.h"
#include "trigger.h"

void init_all() {
    output_init();
    trigger_init();
    pwm_init_module();
    apg_init_module();
}

void abort_all(void) {
    trigger_abort();
    pwm_abort();
    apg_abort();
}

void reset_to_defaults_all(void) {
    g_output_state.enabled = false;
    output_apply_state();
    abort_all();
    init_all();
}

void main_core1_entry(void) {
    init_all();

    while (true) {
        tight_loop_contents();
    }
}
