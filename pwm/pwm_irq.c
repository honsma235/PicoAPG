/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * PWM IRQ Handler
 *
 * Implements cycle-synchronized PWM waveform updates via hardware PWM wrap IRQ.
 * This ensures glitch-free duty cycle updates at PWM cycle boundaries.
 *
 * Functions are __force_inline and placed in SRAM to minimize IRQ latency.
 * Executing from flash proved unreliable at high frequencies, likely due to flash access time (even with XIP cache).
 */

#include "pwm_irq.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/platform.h"
#include "pwm_core1.h"
#include "pwm_gpio.h"
#include <math.h>
#include <stdio.h>

static int g_pwm_irq_slice = -1;

static float g_phase_duty[3];

/* Quarter-wave sine LUT (0..pi/2) with endpoint duplicate for interpolation safety. */
#define PWM_SINE_LUT_SIZE 256
static float g_sin_lut_90[PWM_SINE_LUT_SIZE + 1];
static bool g_sin_lut_initialized = false;

/* Phase offsets in fixed-point 0..2^32 space */
#define PWM_PHASE_OFFSET_120 0x55555555u /* 120 deg */
#define PWM_PHASE_OFFSET_180 0x80000000u /* 180 deg */

static void init_sin_lut(void) {
    if (g_sin_lut_initialized) {
        return;
    }
    for (int i = 0; i <= PWM_SINE_LUT_SIZE; i++) {
        float angle = ((float)i * (float)M_PI_2) / (float)PWM_SINE_LUT_SIZE;
        g_sin_lut_90[i] = sinf(angle);
    }
    g_sin_lut_initialized = true;
}

/* Convert uint32 phase (full turn = 2^32) to sine using quadrant folding and linear interp. */
static __force_inline float get_sin_fixed(uint32_t phase) {
    uint8_t quadrant = (uint8_t)(phase >> 30);
    uint32_t phase_in_quadrant = phase & 0x3FFFFFFFu; /* lower 30 bits */

    uint32_t idx = phase_in_quadrant >> 22;                                    /* top 8 bits -> 0..255 */
    float frac = (float)(phase_in_quadrant & 0x3FFFFFu) * (1.0f / 4194304.0f); /* 1/2^22 */

    float val;
    if (quadrant == 0 || quadrant == 2) {
        /* Forward lookup for 0-90 and 180-270 */
        float a = g_sin_lut_90[idx];
        float b = g_sin_lut_90[idx + 1];
        val = a + (b - a) * frac;
    } else {
        /* Reverse lookup for 90-180 and 270-360 */
        idx = PWM_SINE_LUT_SIZE - 1 - idx;
        float a = g_sin_lut_90[idx + 1];
        float b = g_sin_lut_90[idx];
        val = a + (b - a) * frac;
    }

    return (quadrant >= 2) ? -val : val;
}

static __force_inline void calculate_duties(void) {
    bool dirty = g_pwm_config.reload_runtime_param; /* Check if runtime parameters were updated */
    g_pwm_config.reload_runtime_param = false;      /* Clear dirty flag immediately */
    if (g_pwm_config.control_mode == PWM_CONTROL_DUTY && dirty) {
        /* Direct duty control - use phase_duty values directly */
        g_phase_duty[0] = g_pwm_config.phase_duty[0];
        g_phase_duty[1] = g_pwm_config.phase_duty[1];
        g_phase_duty[2] = g_pwm_config.phase_duty[2];
        return;
    }

    if (g_pwm_config.op_mode == PWM_MODE_ONEPH) {
        /* No angle-based modulation in single-phase mode. Exit early */
        return;
    }

    /* Update phase accumulator and delta on demand */
    if (g_pwm_config.control_mode == PWM_CONTROL_MOD_SPEED) {
        if (dirty) {
            /* MOD_SPEED: recalc delta phase */
            double delta = (double)g_pwm_config.phase_speed_hz / (double)g_pwm_config.frequency_hz * 4294967296.0; /* 2^32 */
            g_pwm_config.delta_phase = (int32_t)delta;
        }
        g_pwm_config.phase_acc += (uint32_t)g_pwm_config.delta_phase; /* wraps naturally */
    } else if (dirty) {
        /* MOD_ANGLE: refresh phase from configured angle */
        /* Assumes 0 <= phase_angle_deg < 360 */
        g_pwm_config.phase_acc = (uint32_t)(g_pwm_config.phase_angle_deg * (4294967296.0f / 360.0f)); /* 2^32 / 360 */
    }

    /* Calculate phase duties based on current accumulator and modulation index */
    switch (g_pwm_config.op_mode) {
    case PWM_MODE_TWOPH: {
        float sin_a = get_sin_fixed(g_pwm_config.phase_acc);
        float sin_b = get_sin_fixed(g_pwm_config.phase_acc + PWM_PHASE_OFFSET_180);
        g_phase_duty[0] = 0.5f + (0.5f * g_pwm_config.mod_index * sin_a);
        g_phase_duty[1] = 0.5f + (0.5f * g_pwm_config.mod_index * sin_b);
        break;
    }
    case PWM_MODE_THREEPH: {
        float sin_a = get_sin_fixed(g_pwm_config.phase_acc);
        float sin_b = get_sin_fixed(g_pwm_config.phase_acc + PWM_PHASE_OFFSET_120);
        float sin_c = get_sin_fixed(g_pwm_config.phase_acc - PWM_PHASE_OFFSET_120);
        g_phase_duty[0] = 0.5f + (0.5f * g_pwm_config.mod_index * sin_a);
        g_phase_duty[1] = 0.5f + (0.5f * g_pwm_config.mod_index * sin_b);
        g_phase_duty[2] = 0.5f + (0.5f * g_pwm_config.mod_index * sin_c);
        break;
    }
    default:
        /* Unsupported mode for modulation - should not happen, set duties to 0.5 */
        g_phase_duty[0] = 0.5f;
        g_phase_duty[1] = 0.5f;
        g_phase_duty[2] = 0.5f;
        break;
    }
}

static __force_inline void clip_duties(void) {
    for (int phase = 0; phase < g_pwm_config.op_mode; phase++) {
        /* Clip duty to min_duty and 1.0 - min_duty */
        if (g_phase_duty[phase] < g_pwm_config.min_duty_with_deadtime) {
            g_phase_duty[phase] = g_pwm_config.min_duty_with_deadtime;
        } else if (g_phase_duty[phase] > (1.0f - g_pwm_config.min_duty_with_deadtime)) {
            g_phase_duty[phase] = 1.0f - g_pwm_config.min_duty_with_deadtime;
        }
    }
}

static __force_inline void set_duties(void) {
    /* Set level values with deadtime */
    for (int phase = 0; phase < g_pwm_config.op_mode; phase++) {
        /* Convert to register values */
        uint16_t level = (uint16_t)(g_phase_duty[phase] * (float)g_pwm_config.max_counter);
        pwm_set_chan_level(g_pwm_config.phase[phase].ls_slice, g_pwm_config.phase[phase].ls_channel, level + g_pwm_config.deadtime_counts_ls);
        pwm_set_chan_level(g_pwm_config.phase[phase].hs_slice, g_pwm_config.phase[phase].hs_channel, level - g_pwm_config.deadtime_counts_hs);
    }
}

/**
 * Check if waveform generation should terminate based on burst type
 * Returns true if NCYCLES limit reached or DURATION timeout exceeded
 */
static __force_inline bool is_burst_termination_required(void) {
    switch (g_pwm_config.burst.type) {
    case BURST_MODE_NCYCLES: {
        /* NCYCLES mode: stop after configured number of cycles */
        g_pwm_config.burst_cycle_counter++;
        return (g_pwm_config.burst_cycle_counter >= g_pwm_config.burst.ncycles);
    }
    case BURST_MODE_DURATION: {
        /* DURATION mode: stop after configured timeout */
        g_pwm_config.burst_ticks_counter += g_pwm_config.burst_ticks_per_cycle;
        return (g_pwm_config.burst_ticks_counter >= g_pwm_config.burst_ticks_timeout);
    }
    default:
        /* CONTINUOUS or unknown: never terminate here */
        return false;
    }
}

/**
 * Transition from RUNNING to IDLE state
 * Disables PWM hardware and applies idle states (or Hi-Z) to all outputs
 */
static __force_inline void transition_to_idle(void) {
    /* Stop PWM */
    pwm_set_mask_enabled(0);

    for (uint8_t phase = 0; phase < g_pwm_config.op_mode; phase++) {
        pwm_phase_config_t *phase_cfg = &g_pwm_config.phase[phase];

        /* set idle level */
        pwm_set_chan_level(phase_cfg->ls_slice, phase_cfg->ls_channel, phase_cfg->ls_idle ? g_pwm_config.max_counter : 0);
        pwm_set_chan_level(phase_cfg->hs_slice, phase_cfg->hs_channel, phase_cfg->hs_idle ? g_pwm_config.max_counter : 0);

        /* reset counters */
        pwm_set_counter(phase_cfg->ls_slice, 0);
        pwm_set_counter(phase_cfg->hs_slice, 0);
    }

    /* Start/Stop PWM to reload double buffered registers to apply idle levels */
    pwm_set_mask_enabled(g_pwm_config.pwm_enable_mask);
    pwm_set_mask_enabled(0);

    g_pwm_config.state = PWM_STATE_IDLE;
    g_pwm_config.running = false;
    /* Clear any pending triggers to prevent immediate restart */
    g_pwm_config.trigger.bus_triggered = false;
}

/* Shared body for IRQ and pre-prime call; prime_run runs even if not yet running. */
static void __not_in_flash_func(pwm_irq_run)(bool prime_run) {

    gpio_put(2, 1); /* Toggle GPIO for timing measurement (scope) */
    bool running = g_pwm_config.running;

    if (!prime_run && (is_burst_termination_required() || !running)) {
        /* burst done or transition to idle requested*/
        gpio_put(2, 0);
        transition_to_idle();
        return;
    }
    if (!prime_run && !running) {
        return;
    }

    calculate_duties();
    clip_duties();
    set_duties();

    gpio_put(2, 0); /* Toggle GPIO for timing measurement (scope) */
}

/**
 * PWM wrap IRQ handler - called at PWM cycle boundaries
 *
 * Reads config (if dirty), calculates duties, updates PWM compare registers.
 * Optimized for minimal IRQ latency.
 */
static void __isr __not_in_flash_func(pwm_wrap_irq_handler)(void) {
    /* Clear interrupt flag */
    pwm_clear_irq(g_pwm_irq_slice);
    pwm_irq_run(false);
}

/* Prime PWM compare levels once before enabling the slice to avoid a cold first IRQ. */
void __not_in_flash_func(pwm_irq_prime)(void) {
    g_pwm_config.reload_runtime_param = true; /* Force reload on prime */
    pwm_irq_run(true);
    g_pwm_config.phase_acc = 0; /* Reset phase accumulator to start with 0° on first real IRQ */
}

/**
 * Initialize PWM IRQ handler
 */
void pwm_irq_setup(int slice) {

    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_put(2, 0);

    init_sin_lut();

    if (slice == g_pwm_irq_slice) {
        return; /* no change */
    }

    /* If already initialized, disable previous IRQ */
    if (g_pwm_irq_slice >= 0) {
        irq_set_enabled(PWM_IRQ_WRAP_0, false);
        pwm_set_irq_enabled((uint)g_pwm_irq_slice, false);
        g_pwm_irq_slice = -1;
    }

    if (slice < 0) {
        return;
    }

    /* Register IRQ handler for PWM slice */
    g_pwm_irq_slice = slice;
    pwm_clear_irq((uint)slice);
    pwm_set_irq_enabled((uint)slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, pwm_wrap_irq_handler);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);
}
