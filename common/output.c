/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 *
 * Central output state control
 */

#include "hardware/gpio.h"

#include "output.h"
#include "pwm/pwm_gpio.h"
#include "apg/apg.h"

output_state_t g_output_state;

void output_init(void) {
    g_output_state.enabled = false;
    output_apply_state();
}

void output_apply_state(void) {
    /* Centralize the output state control */
    pwm_outputs_update();
    apg_outputs_update();
}

void output_detach(int gpio) {
    // Set input enable off, output disable on
    hw_write_masked(&pads_bank0_hw->io[gpio], PADS_BANK0_GPIO0_OD_BITS,
                    PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
}