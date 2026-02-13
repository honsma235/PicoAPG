/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * PWM Configuration & Shared Data Structures
 *
 * This file defines the shared configuration structure between Core0 and Core1.
 * All access to pwm_config_t must be spinlock-protected.
 */

#ifndef PWM_CONFIG_H
#define PWM_CONFIG_H

#include "../scpi_server/scpi_commands_gen.h"
#include "pico/sync.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * PWM State Machine States (Internal)
 * =========================================================================
 */
typedef enum {
    PWM_STATE_IDLE,   /* Idle state, no PWM generation, waiting for trigger signal */
    PWM_STATE_RUNNING /* Active PWM generation */
} pwm_state_t;

/* =========================================================================
 * Per-Phase GPIO Configuration
 * =========================================================================
 */
typedef struct {
    int gpio_ls;        /* Low-side GPIO pin number (0-29, -1 = unassigned) */
    int gpio_hs;        /* High-side GPIO pin number (0-29, -1 = unassigned) */
    bool ls_inverted;   /* Low-side output inversion */
    bool hs_inverted;   /* High-side output inversion */
    bool ls_idle;       /* Low-side idle state */
    bool hs_idle;       /* High-side idle state */
    uint8_t ls_slice;   /* PWM slice for low-side GPIO */
    uint8_t ls_channel; /* PWM channel for low-side GPIO */
    uint8_t hs_slice;   /* PWM slice for high-side GPIO */
    uint8_t hs_channel; /* PWM channel for high-side GPIO */
} pwm_phase_config_t;

/* =========================================================================
 * Trigger Configuration
 * =========================================================================
 */
typedef struct {
    TRIGGER_SOURCE_TRG_SOURCE_t source; /* Current trigger source */

    /* Trigger timing */
    float trigger_delay_sec; /* Delay from trigger to PWM start */

    /* Trigger flags (set by trigger source handlers) */
    volatile bool bus_triggered; /* Set by BUS mechanism */
} pwm_trigger_config_t;

/* =========================================================================
 * Burst Mode Configuration
 * =========================================================================
 */
typedef struct {
    SOURCE_BURST_TYPE_BURST_MODE_t type; /* CONTINUOUS, NCYCLES, or DURATION mode */
    uint32_t ncycles;                    /* Number of cycles to generate in BURST mode */
    float idle_delay_sec;                /* Delay between end of burst and next trigger */
    float timeout_sec;                   /* Max time to run in GATED mode (0 = disabled) */
} pwm_burst_config_t;

/* =========================================================================
 * Main PWM Configuration Structure
 *
 * Protected by critical_section. Access pattern:
 *   Core0: Read via critical_section, write via critical_section
 *   Core1: Read via critical_section, write only from Core1 main loop after acquiring critical_section
 *
 * Per SDK docs (page 399): critical_section provides "short-lived mutual exclusion
 * safe for IRQ and multi-core" using spinlock + IRQ disable.
 * =========================================================================
 */
typedef struct {
    bool outputs_enabled; /* Global output enable flag (:OUTPut:STATe) */

    /* Dirty flag to signal hardware config changes */
    volatile bool hardware_config_dirty; /* Set by Core0 on hardware param write, cleared by Core1 after reconfiguration */
    volatile bool reload_runtime_param;  /* Set by Core0 on runtime param write, cleared by IRQ */

    /* State management */
    volatile pwm_state_t state; /* Current PWM state machine state */
    volatile bool running;      /* True when PWM is actively generating waveforms */

    /* Output mode and control */
    SOURCE_PWM_MODE_PWM_MODE_t op_mode;            /* OFF, ONEPH, TWOPH, THREEPH */
    SOURCE_PWM_CONTROL_PWM_CONTROL_t control_mode; /* DUTY, MOD_ANGLE, MOD_SPEED */

    /* Static configuration */
    float frequency_hz;           /* PWM carrier frequency in Hz */
    float deadtime;               /* Deadtime between HS/LS switching in seconds */
    float min_duty;               /* Minimum duty cycle constraint (0.0 - 0.2) */
    pwm_phase_config_t phase[3];  /* Phases 1-3 configuration */
    pwm_trigger_config_t trigger; /* Trigger source and settings */
    pwm_burst_config_t burst;     /* Burst mode settings */

    /* Runtime parameters */
    float phase_duty[3];   /* Per-phase duty cycle (0.0 - 1.0) for phases 1-3 */
    float mod_index;       /* Modulation index (0.0 - 1.0) */
    float phase_angle_deg; /* Phase angle in degrees (wraps at 360) */
    float phase_speed_hz;  /* Phase rotation speed in Hz */

    /* pre-calculated values */
    uint32_t pwm_enable_mask;     /* Bitmask of active PWM slices for current op_mode */
    float min_duty_with_deadtime; /* Minimum duty cycle plus half deadtime as fraction of period, for clipping */
    uint16_t max_counter;         /* Calculated PWM max_counter value based on frequency */
    uint16_t deadtime_counts_ls;  /* Pre-calculated deadtime in level counts */
    uint16_t deadtime_counts_hs;  /* Pre-calculated deadtime in level counts */

    /* Burst state*/
    uint32_t burst_cycle_counter;   /* Counter of PWM cycles since start */
    uint64_t burst_ticks_counter;   /* Counter of system clock ticks since start */
    uint64_t burst_ticks_timeout;   /* System clock ticks until burst timeout */
    uint32_t burst_ticks_per_cycle; /* System clock ticks per PWM cycle */

    /* Fixed-point phase accumulator state for MOD_SPEED mode*/
    uint32_t phase_acc;
    int32_t delta_phase; /* Signed to allow negative rotation speeds */

} pwm_config_t;

/* =========================================================================
 * Global PWM Configuration Instance
 * =========================================================================
 */
extern pwm_config_t g_pwm_config;

/* =========================================================================
 * Helper Functions
 * =========================================================================
 */

static inline void pwm_config_lock(void) {
    // critical_section_enter_blocking(&g_pwm_config.lock);
}

static inline void pwm_config_unlock(void) {
    // critical_section_exit(&g_pwm_config.lock);
}

/**
 * Initialize PWM configuration to safe defaults
 */
void pwm_config_init(void);

/**
 * Reset PWM configuration to defaults. Used by *RST. Triggers clean trnsition to IDLE state if necessary.
 */
void pwm_config_reset_to_defaults(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_CONFIG_H */
