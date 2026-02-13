/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "pwm_core1.h"

#include <math.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include "pwm_gpio.h"
#include "pwm_irq.h"

extern pwm_config_t g_pwm_config;

/**
 * Check if trigger event occurred and clear it
 * Returns true if BUS trigger was set, and clears the flag atomically
 */
static inline bool check_trigger_event(void) {
    bool triggered = g_pwm_config.trigger.bus_triggered;
    if (triggered) {
        g_pwm_config.trigger.bus_triggered = false;
    }
    return triggered;
}

/**
 * Transition from IDLE to RUNNING state
 * Enables PWM hardware and applies PWM GPIO function
 */
static inline void transition_to_running(void) {
    g_pwm_config.state = PWM_STATE_RUNNING;
    g_pwm_config.running = true;

    /* Prime PWM compare levels once before enabling the slice to avoid a cold first IRQ. */
    pwm_irq_prime();

    /* Enable PWM slices */
    pwm_set_mask_enabled(g_pwm_config.pwm_enable_mask);
}

static inline void reconfigure_hardware(void) {
    /* Re-initialize PWM hardware */
    pwm_init_hardware();

    /* Pre-calculate internal values */
    g_pwm_config.min_duty_with_deadtime = g_pwm_config.min_duty + (0.5f * g_pwm_config.deadtime * g_pwm_config.frequency_hz); /* Minimum duty plus half deadtime as fraction of period */
    uint16_t deadtime_counts = roundf(g_pwm_config.deadtime * g_pwm_config.frequency_hz * (float)g_pwm_config.max_counter);
    g_pwm_config.deadtime_counts_ls = deadtime_counts / 2;
    g_pwm_config.deadtime_counts_hs = (deadtime_counts + 1) / 2;
    g_pwm_config.burst_ticks_timeout = round((double)g_pwm_config.burst.timeout_sec * clock_get_hz(clk_sys));
    g_pwm_config.burst_ticks_per_cycle = roundf(clock_get_hz(clk_sys) / g_pwm_config.frequency_hz);

    /* reset runtime state */
    g_pwm_config.burst_cycle_counter = 0;
    g_pwm_config.burst_ticks_counter = 0;
    g_pwm_config.phase_acc = 0;

    g_pwm_config.hardware_config_dirty = false;

    /* Re-initialize PWM IRQ handler for first low-side slice */
    pwm_irq_setup(g_pwm_config.phase[0].ls_slice);
}

void pwm_core1_main(void) {

    /* Initialize PWM hardware */
    reconfigure_hardware();

    /* Main PWM execution loop - trigger coordinator */
    while (1) {
        /* Read current state */
        pwm_state_t state = g_pwm_config.state;
        bool running = g_pwm_config.running;
        bool hardware_config_dirty = g_pwm_config.hardware_config_dirty;
        // float trigger_delay_sec = g_pwm_config.trigger.trigger_delay_sec;

        /* Apply pending hardware reconfiguration while IDLE */
        if (state == PWM_STATE_IDLE && hardware_config_dirty) {
            reconfigure_hardware();
        }

        /* Handle state machine: IDLE -> trigger detection -> RUNNING */
        if (state == PWM_STATE_IDLE && !running) {
            /* Check for trigger event */
            if (check_trigger_event()) {
                /* Apply trigger delay if configured */
                // if (trigger_delay_sec > 0.0f) {
                //     uint32_t delay_us = (uint32_t)(trigger_delay_sec *
                //     1000000.0f); busy_wait_us(delay_us);
                // }
                /* Transition to RUNNING state */
                transition_to_running();
            }
        }

        /* Transition to IDLE is handled by IRQ */

        tight_loop_contents();
    }
}
