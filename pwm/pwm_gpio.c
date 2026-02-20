/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

#include "pwm_gpio.h"


void __no_inline_not_in_flash_func(pwm_set_idle_state)(void) {
    for (uint8_t phase = 0; phase < g_pwm_config.op_mode; phase++) {
        pwm_phase_config_t *phase_cfg = &g_pwm_config.phase[phase];
        if (phase_cfg->gpio_ls >= 0) {
            /* reset counters */
            pwm_set_counter(phase_cfg->ls_slice, 0);
            /* Update idle levels, set CC to idle values */
            pwm_set_chan_level(phase_cfg->ls_slice, phase_cfg->ls_channel, phase_cfg->ls_idle ? g_pwm_config.max_counter : 0);
        }
        if (phase_cfg->gpio_hs >= 0) {
            /* reset counters */
            pwm_set_counter(phase_cfg->hs_slice, 0);
            /* Update idle levels, set CC to idle values, inverted for high-side */
            pwm_set_chan_level(phase_cfg->hs_slice, phase_cfg->hs_channel, phase_cfg->hs_idle ? 0 : g_pwm_config.max_counter);
        }
    }

    irq_set_enabled(PWM_IRQ_WRAP_0, false);
    /* Start/Stop PWM to reload double buffered registers to apply idle levels */
    __dsb(); /* ensure registers are loaded before enabling */
    pwm_set_mask_enabled(g_pwm_config.pwm_enable_mask);
    __dsb();
    __nop(); /* small delay to ensure registers are loaded before disabling */
    pwm_set_mask_enabled(0);
}

/**
 * Force all GPIO outputs to high-impedance
 * Sets all configured GPIO pins to Hi-Z state (disabled output, disabled input)
 */
void pwm_outputs_detach(void) {
    /* Loop over all configured user GPIO pins */
    for (uint8_t phase = 0; phase < 3; phase++) {
        int gpio_ls = g_pwm_config.phase[phase].gpio_ls;
        int gpio_hs = g_pwm_config.phase[phase].gpio_hs;

        if (gpio_ls != -1) {
            // Set input enable off, output disable on
            hw_write_masked(&pads_bank0_hw->io[gpio_ls], PADS_BANK0_GPIO0_OD_BITS,
                            PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
        }

        if (gpio_hs != -1) {
            // Set input enable off, output disable on
            hw_write_masked(&pads_bank0_hw->io[gpio_hs], PADS_BANK0_GPIO0_OD_BITS,
                            PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
        }
    }
}

void pwm_outputs_attach(void) {
    if (g_pwm_config.op_mode == PWM_MODE_OFF) {
        return;
    }

    /* Loop over all configured user GPIO pins */
    for (uint8_t phase = 0; phase < 3; phase++) {
        int gpio_ls = g_pwm_config.phase[phase].gpio_ls;
        int gpio_hs = g_pwm_config.phase[phase].gpio_hs;

        if (gpio_ls != -1) {
            gpio_set_function(gpio_ls, GPIO_FUNC_PWM);
        }

        if (gpio_hs != -1) {
            gpio_set_function(gpio_hs, GPIO_FUNC_PWM);
        }
    }
}

bool pwm_gpio_in_use(int gpio) {
    if (gpio < 0)
        return false;
    /* PWM channels on GPIO 0 to 15 repeat on GPIO 16 through 31 */
    /* So check everything modulo 16 */
    gpio %= 16;
    for (unsigned int i = 0; i < 3; i++) {
        if ((g_pwm_config.phase[i].gpio_ls % 16) == gpio || (g_pwm_config.phase[i].gpio_hs % 16) == gpio)
            return true;
    }
    return false;
}