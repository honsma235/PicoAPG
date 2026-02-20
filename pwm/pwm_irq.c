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

#include <math.h>

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/platform.h"

#include "common/trigger.h"
#include "pwm.h"
#include "pwm_gpio.h"
#include "pwm_irq.h"

#define PWM_IRQ_DEBUG_ENABLE 0 /* Set to 1 to enable GPIO toggling for IRQ timing measurement (scope) */
#define PWM_IRQ_DEBUG_GPIO 2 /* GPIO toggled at start/end of IRQ for timing measurement (scope) */
#if PWM_IRQ_DEBUG_ENABLE
#define PWM_IRQ_DEBUG_SET(x) do { gpio_put(PWM_IRQ_DEBUG_GPIO, (x)); } while(0)
#define PWM_IRQ_DEBUG_INIT() do { gpio_init(PWM_IRQ_DEBUG_GPIO); gpio_set_dir(PWM_IRQ_DEBUG_GPIO, GPIO_OUT); gpio_put(PWM_IRQ_DEBUG_GPIO, false);} while(0)
#else
#define PWM_IRQ_DEBUG_SET(x) do { } while(0)
#define PWM_IRQ_DEBUG_INIT() do { } while(0)
#endif


static float g_phase_duty[3];
static uint32_t g_phase_acc = 0;            /* Fixed-point phase accumulator state for MOD_SPEED mode*/
static int32_t g_delta_phase;               /* Signed to allow negative rotation speeds */
static uint32_t g_burst_ncycle_counter = 0; /* Counter of PWM cycles since start */
static uint32_t g_burst_ncycle_snapshot = 0;

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

/* Convert uint32 phase (full turn [0..2pi[ = 2^32) to sine using quadrant folding and linear interp. */
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
    switch (g_pwm_config.control_mode) {
    case PWM_CONTROL_DUTY:
        if (dirty) {
            /* Direct duty control - use phase_duty values directly */
            g_phase_duty[0] = g_pwm_config.phase_duty[0];
            g_phase_duty[1] = g_pwm_config.phase_duty[1];
            g_phase_duty[2] = g_pwm_config.phase_duty[2];
        }
        /* No angle-based modulation, exit early*/
        return;

    case PWM_CONTROL_MOD_ANGLE:
        if (dirty) {
            /* MOD_ANGLE: refresh phase from configured angle */
            /* Assumes 0 <= phase_angle_deg < 360 */
            g_phase_acc = (uint32_t)(g_pwm_config.phase_angle_deg * (4294967296.0f / 360.0f)); /* 2^32 / 360 */
        }
        break;

    case PWM_CONTROL_MOD_SPEED:
        if (dirty) {
            /* MOD_SPEED: recalc delta phase */
            double delta = (double)g_pwm_config.phase_speed_hz / (double)g_pwm_config.frequency_hz * 4294967296.0; /* 2^32 */
            g_delta_phase = (int32_t)delta;
        }
        g_phase_acc += (uint32_t)g_delta_phase; /* wraps naturally */
        break;

    default:
        break;
    }

    /* Calculate phase duties based on current accumulator and modulation index */
    switch (g_pwm_config.op_mode) {
    case PWM_MODE_TWOPH: {
        float sin_a = get_sin_fixed(g_phase_acc);
        float sin_b = get_sin_fixed(g_phase_acc + PWM_PHASE_OFFSET_180);
        g_phase_duty[0] = 0.5f + (0.5f * g_pwm_config.mod_index * sin_a);
        g_phase_duty[1] = 0.5f + (0.5f * g_pwm_config.mod_index * sin_b);
        break;
    }
    case PWM_MODE_THREEPH: {
        float sin_a = get_sin_fixed(g_phase_acc);
        float sin_b = get_sin_fixed(g_phase_acc + PWM_PHASE_OFFSET_120);
        float sin_c = get_sin_fixed(g_phase_acc - PWM_PHASE_OFFSET_120);
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
        }
        if (g_phase_duty[phase] > (1.0f - g_pwm_config.min_duty_with_deadtime)) {
            g_phase_duty[phase] = 1.0f - g_pwm_config.min_duty_with_deadtime;
        }
    }
}

static __force_inline void set_duties(void) {
    /* Set level values with deadtime */
    for (int phase = 0; phase < g_pwm_config.op_mode; phase++) {
        pwm_phase_config_t *phase_cfg = &g_pwm_config.phase[phase];
        /* Convert to register values */
        uint16_t level = (uint16_t)(g_phase_duty[phase] * (float)g_pwm_config.max_counter);
        if (phase_cfg->gpio_ls >= 0) {
            pwm_set_chan_level(phase_cfg->ls_slice, phase_cfg->ls_channel, level - g_pwm_config.deadtime_counts_ls);
        }
        if (phase_cfg->gpio_hs >= 0) {
            pwm_set_chan_level(phase_cfg->hs_slice, phase_cfg->hs_channel, level + g_pwm_config.deadtime_counts_hs);
        }
    }
}

/**
 * Check if waveform generation should terminate based on burst type
 * Returns true if NCYCLES limit reached
 */
static __force_inline bool is_burst_termination_required(void) {
    if (g_trigger_config.burst_type == BURST_MODE_NCYCLES) {
        /* NCYCLES mode: stop after configured number of cycles */
        g_burst_ncycle_counter++;
        return (g_burst_ncycle_counter >= g_burst_ncycle_snapshot);
    }
    /* Other burst modes not handled here */
    return false;
}

/* Shared body for IRQ and pre-prime call; prime_run runs even if not yet running. */
static void __no_inline_not_in_flash_func(pwm_irq_run)(bool prime_run) {
    PWM_IRQ_DEBUG_SET(true);
    
    if (!prime_run && (is_burst_termination_required() || (g_pwm_config.state != PWM_STATE_RUNNING))) {
        /* burst done or transition to idle requested*/
        PWM_IRQ_DEBUG_SET(false);
        pwm_abort();
        return;
    }

    calculate_duties();
    clip_duties();
    set_duties();

    PWM_IRQ_DEBUG_SET(false);
}

/**
 * PWM wrap IRQ handler - called at PWM cycle boundaries
 *
 * Reads config (if dirty), calculates duties, updates PWM compare registers.
 * Optimized for minimal IRQ latency.
 */
void __isr __not_in_flash_func(pwm_wrap_irq_handler)(void) {
    /* Clear interrupt flag */
    pwm_clear_irq(g_pwm_config.pwm_irq_slice);
    pwm_irq_run(false);
}

/* Prime PWM compare levels once before enabling the slice to avoid a cold first IRQ. */
void __no_inline_not_in_flash_func(pwm_irq_prime)(void) {
    /**
     * Initialize PWM IRQ handler
     */
    pwm_irq_run(true);
    g_phase_acc = 0;                                          /* Reset phase accumulator to start with 0° on first real IRQ */
    g_burst_ncycle_counter = 0;                               /* Reset burst cycle counter on prime */
    g_burst_ncycle_snapshot = g_trigger_config.burst_ncycles; /* Snapshot NCYCLES count at start of burst */
}

void pwm_irq_init(void) {
    PWM_IRQ_DEBUG_INIT();
    init_sin_lut();
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, pwm_wrap_irq_handler);
}
