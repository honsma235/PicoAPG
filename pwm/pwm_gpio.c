/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "pwm_gpio.h"

#include <math.h>
#include <stddef.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/platform.h"

extern pwm_config_t g_pwm_config;

static inline void pwm_set_channel_polarity(uint slice_num, uint chan, bool inverted) {
    hw_write_masked(&pwm_hw->slice[slice_num].csr,
                    bool_to_bit(inverted) << (chan ? PWM_CH0_CSR_B_INV_LSB : PWM_CH0_CSR_A_INV_LSB),
                    chan ? PWM_CH0_CSR_B_INV_BITS : PWM_CH0_CSR_A_INV_BITS);
}

void pwm_init_hardware(void) {
    float clock_hz = clock_get_hz(clk_sys); /* system clock */

    /* Calculate required divider for max resolution */
    /* only use interger divider, not fractional */
    /* Use slightly less than 2^16-1 to avoid rounding errors */
    uint16_t clkdiv = ceilf(clock_hz / (g_pwm_config.frequency_hz * 2.0f * 65534.0f));
    if (clkdiv > 255)
        clkdiv = 255; /* max divider is 255 */
    /* Calculate max_counter (= max level = wrap + 1) based on frequency and clkdiv */
    g_pwm_config.max_counter = (uint16_t)roundf(clock_hz / (g_pwm_config.frequency_hz * 2.0f * (float)clkdiv));

    /* Initialize each phase's GPIO pins and PWM slices and calculate enable mask */
    uint32_t mask = 0;
    for (uint8_t phase = 0; phase < g_pwm_config.op_mode; phase++) {
        pwm_phase_config_t *phase_cfg = &g_pwm_config.phase[phase];

        uint8_t gpio_ls = phase_cfg->gpio_ls;
        uint8_t gpio_hs = phase_cfg->gpio_hs;

        /* Configure low-side GPIO */
        phase_cfg->ls_slice = pwm_gpio_to_slice_num(gpio_ls);
        phase_cfg->ls_channel = pwm_gpio_to_channel(gpio_ls);

        /* Configure high-side GPIO */
        phase_cfg->hs_slice = pwm_gpio_to_slice_num(gpio_hs);
        phase_cfg->hs_channel = pwm_gpio_to_channel(gpio_hs);

        /* Calculate enable mask for PWM slices */
        mask |= (1u << phase_cfg->ls_slice);
        mask |= (1u << phase_cfg->hs_slice);

        /* Apply alignment mode phase-correct */
        pwm_set_phase_correct(phase_cfg->ls_slice, true);
        pwm_set_phase_correct(phase_cfg->hs_slice, true);

        /* Set wrap value based on frequency */
        pwm_set_wrap(phase_cfg->ls_slice, g_pwm_config.max_counter - 1);
        pwm_set_wrap(phase_cfg->hs_slice, g_pwm_config.max_counter - 1);

        /* Set clock divider, integer only */
        pwm_set_clkdiv_int_frac4(phase_cfg->ls_slice, clkdiv, 0);
        pwm_set_clkdiv_int_frac4(phase_cfg->hs_slice, clkdiv, 0);

        /* Set polarity based on inversion config */
        pwm_set_channel_polarity(phase_cfg->ls_slice, phase_cfg->ls_channel, phase_cfg->ls_inverted);
        /* High side is normally inverted */
        pwm_set_channel_polarity(phase_cfg->hs_slice, phase_cfg->hs_channel, !phase_cfg->hs_inverted);

        /* reset counters */
        pwm_set_counter(phase_cfg->ls_slice, 0);
        pwm_set_counter(phase_cfg->hs_slice, 0);

        /* Set idle levels */
        pwm_set_chan_level(phase_cfg->ls_slice, phase_cfg->ls_channel, phase_cfg->ls_idle ? g_pwm_config.max_counter : 0);
        pwm_set_chan_level(phase_cfg->hs_slice, phase_cfg->hs_channel, phase_cfg->hs_idle ? g_pwm_config.max_counter : 0);
    }

    /* Store PWM enable mask */
    g_pwm_config.pwm_enable_mask = mask;

    /* Start/Stop PWM to reload double buffered registers to apply idle levels */
    pwm_set_mask_enabled(mask);
    pwm_set_mask_enabled(0);
}

/**
 * Force all GPIO outputs to high-impedance
 * Sets all configured GPIO pins to Hi-Z state (disabled output, disabled input)
 */
static inline void pwm_gpio_set_hi_z_all(void) {
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

static inline void pwm_gpio_set_pwm_all(void) {
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

/**
 * Apply GPIO state based on PWM and output configuration
 * Sets GPIO function to PWM or Hi-Z based on outputs_enabled and op_mode
 */

void pwm_gpio_apply_output_state(void) {
    SOURCE_PWM_MODE_PWM_MODE_t op_mode = g_pwm_config.op_mode;
    bool outputs_enabled = g_pwm_config.outputs_enabled;

    /* Outputs disabled or Mode OFF: always Hi-Z */
    if (!outputs_enabled || op_mode == PWM_MODE_OFF) {
        pwm_gpio_set_hi_z_all();
    } else {
        pwm_gpio_set_pwm_all();
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