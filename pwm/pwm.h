/*
 * Unified PWM configuration and control
 */

#ifndef PWM_H
#define PWM_H

#include <stdbool.h>
#include <stdint.h>

#include "../scpi_server/scpi_commands_gen.h"

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

typedef struct {
    volatile bool reload_runtime_param; /* Set by Core0 on runtime param write, cleared by IRQ */

    /* State management */
    volatile pwm_state_t state; /* Current PWM state machine state */
    
    /* Output mode and control */
    SOURCE_PWM_MODE_PWM_MODE_t op_mode;            /* OFF, ONEPH, TWOPH, THREEPH */
    SOURCE_PWM_CONTROL_PWM_CONTROL_t control_mode; /* DUTY, MOD_ANGLE, MOD_SPEED */

    /* Static configuration */
    float frequency_hz;          /* PWM carrier frequency in Hz */
    float deadtime;              /* Deadtime between HS/LS switching in seconds */
    float min_duty;              /* Minimum duty cycle constraint (0.0 - 0.2) */
    pwm_phase_config_t phase[3]; /* Phases 1-3 configuration */

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

    /* Internal IRQ state */
    int pwm_irq_slice; /* PWM slice used for IRQ handling */

} pwm_config_t;

extern pwm_config_t g_pwm_config;

void pwm_init_module(void);

void pwm_update_config(void);

void pwm_trigger_start(void);

/* Immediate abort: stop slices and apply idle levels without waiting for wrap IRQ. */
void pwm_abort(void);

void pwm_core1_start(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_H */