/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "pwm_config.h"
#include <string.h>

/* Global PWM configuration instance */
pwm_config_t g_pwm_config;

/**
 * Apply default configuration values (internal helper)
 * Resets ALL parameters to safe defaults
 */
static void apply_config_defaults(void) {
    /* Outputs and flags */
    g_pwm_config.outputs_enabled = false;
    g_pwm_config.hardware_config_dirty = true;
    g_pwm_config.reload_runtime_param = true;

    /* State management */
    g_pwm_config.state = PWM_STATE_IDLE;
    g_pwm_config.running = false;

    /* Control configuration */
    g_pwm_config.op_mode = PWM_MODE_OFF;
    g_pwm_config.control_mode = PWM_CONTROL_DUTY;

    /* Frequency and timing */
    g_pwm_config.frequency_hz = 10000.0f;
    g_pwm_config.deadtime = 1E-6f;
    g_pwm_config.min_duty = 0.05f;

    /* GPIO configuration - reset to defaults */
    for (int i = 0; i < 3; i++) {
        g_pwm_config.phase[i].gpio_ls = -1; /* Unassigned */
        g_pwm_config.phase[i].gpio_hs = -1; /* Unassigned */
        g_pwm_config.phase[i].ls_inverted = false;
        g_pwm_config.phase[i].hs_inverted = false;
        g_pwm_config.phase[i].ls_idle = false; /* Default idle state: low */
        g_pwm_config.phase[i].hs_idle = false;
        g_pwm_config.phase[i].ls_slice = 0;
        g_pwm_config.phase[i].ls_channel = 0;
        g_pwm_config.phase[i].hs_slice = 0;
        g_pwm_config.phase[i].hs_channel = 1;
    }

    /* Trigger configuration - BUS source only */
    g_pwm_config.trigger.source = TRG_SOURCE_BUS;
    g_pwm_config.trigger.trigger_delay_sec = 0.0f;
    g_pwm_config.trigger.bus_triggered = false;

    /* Burst configuration */
    g_pwm_config.burst.type = BURST_MODE_CONTINUOUS;
    g_pwm_config.burst.ncycles = 1;
    g_pwm_config.burst.idle_delay_sec = 0.0f;
    g_pwm_config.burst.timeout_sec = 0.01f;

    /* Runtime parameters */
    g_pwm_config.phase_duty[0] = 0.5f;
    g_pwm_config.phase_duty[1] = 0.5f;
    g_pwm_config.phase_duty[2] = 0.5f;
    g_pwm_config.mod_index = 0.0f;
    g_pwm_config.phase_angle_deg = 0.0f;
    g_pwm_config.phase_speed_hz = 1.0f;

    /* Pre-calculated values */
    g_pwm_config.pwm_enable_mask = 0;
    g_pwm_config.min_duty_with_deadtime = 0.0f;
    g_pwm_config.max_counter = 0;
    g_pwm_config.deadtime_counts_ls = 0;
    g_pwm_config.deadtime_counts_hs = 0;

    /* Runtime state */
    g_pwm_config.burst_cycle_counter = 0;
    g_pwm_config.burst_ticks_counter = 0;
    g_pwm_config.burst_ticks_timeout = 0;
    g_pwm_config.burst_ticks_per_cycle = 0;
    g_pwm_config.delta_phase = 0;
    g_pwm_config.phase_acc = 0;
}

void pwm_config_init(void) {
    memset(&g_pwm_config, 0, sizeof(pwm_config_t));

    /* Apply defaults */
    apply_config_defaults();
}

void pwm_config_reset_to_defaults(void) {

    /* Step 1: Disable all triggers to prevent re-triggering during reset */
    /* Only BUS trigger source is available */
    g_pwm_config.trigger.source = TRG_SOURCE_BUS;
    g_pwm_config.trigger.bus_triggered = false;

    /* Step 2: Signal PWM stop (running = false, but keep state intact) */
    /* This allows Core1 to detect RUNNING && !running and call transition_to_idle */
    bool was_running = g_pwm_config.running;
    g_pwm_config.running = false;

    // todo: wont work for slow pwm, change to different approach
    /* Step 3: Wait for Core1 to process the stop (graceful transition to IDLE) */
    if (was_running) {
        /* Give Core1 time to detect stop and call transition_to_idle() */
        for (int i = 0; i < 100; i++) {
            sleep_us(1000); /* 1ms per iteration, ~100ms total max wait */

            pwm_state_t current_state = g_pwm_config.state;

            /* Check if Core1 has confirmed IDLE state */
            if (current_state == PWM_STATE_IDLE) {
                break; /* Safe to proceed with reset */
            }
        }
    }

    /* Step 4: Reset all configuration to defaults */
    apply_config_defaults();

    /* At this point:
     * - Only BUS trigger source available (no external/timer/manual)
     * - Trigger bus_triggered flag cleared
     * - PWM stopped and transitioned to IDLE by Core1
     * - All config reset to defaults
     * - Core1 will see hardware_config_dirty = true on next loop
     * - Core1 will call reconfigure_hardware() to reset all HW to defaults
     * - No automatic re-trigger possible (must use :TRIGger or *TRG command)
     */
}
