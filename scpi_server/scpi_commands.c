/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */

/*
 * User SCPI command implementation
 *
 * This file contains custom implementations for SCPI command handlers.
 * These functions override the weak stubs in scpi_commands_gen.c.
 *
 * Implementation Pattern
 * ======================
 *
 * Naming:
 *   - Function names are derived from the SCPI command tokens and are
 *     fully upper-case (with indexed tokens preserved as PHASEN, etc.).
 *
 * Return conventions:
 *   - Handlers shall return SCPI error codes (e.g. SCPI_ERROR_NO_ERROR
 *     on success).
 *
 * SET/EVENT commands:
 *   - Implement custom_<COMMAND>(params...)
 *   - Apply settings or trigger the event and return a SCPI error code.
 *
 * QUERY commands ("?"):
 *   - Implement custom_<COMMAND>_QUERY(out params...)
 *   - Fill output parameters and return a SCPI error code.
 *
 * Indexed commands:
 *   - The first parameter is a const unsigned int indices[] array.
 *   - The number of indices matches the count of <n> tokens in the SCPI
 *     command. Each value is already range-checked by the parser.
 */

#include "pwm/pwm_config.h"
#include "pwm/pwm_gpio.h"
#include "scpi_commands_gen.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Helper macros for common checks */
#define REQUIRE_OUTPUTS_DISABLED()               \
    do {                                         \
        if (g_pwm_config.outputs_enabled) {      \
            return SCPI_ERROR_SETTINGS_CONFLICT; \
        }                                        \
    } while (0)

#define PWM_REQUIRE_NOT_RUNNING()                \
    do {                                         \
        if (g_pwm_config.running) {              \
            return SCPI_ERROR_SETTINGS_CONFLICT; \
        }                                        \
    } while (0)

int custom_SOURCE_PWM_MODE(SOURCE_PWM_MODE_PWM_MODE_t mode) {
    REQUIRE_OUTPUTS_DISABLED();
    PWM_REQUIRE_NOT_RUNNING();
    if (mode == PWM_MODE_ONEPH && g_pwm_config.control_mode != PWM_CONTROL_DUTY) {
        // Only DUTY control is valid in ONEPH mode
        return SCPI_ERROR_SETTINGS_CONFLICT;
    }
    g_pwm_config.op_mode = mode;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MODE_QUERY(SOURCE_PWM_MODE_PWM_MODE_t *mode) {
    *mode = g_pwm_config.op_mode;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_CONTROL(SOURCE_PWM_CONTROL_PWM_CONTROL_t control) {
    PWM_REQUIRE_NOT_RUNNING();
    if (control != PWM_CONTROL_DUTY && g_pwm_config.op_mode == PWM_MODE_ONEPH) {
        // Only DUTY control is valid in ONEPH mode
        return SCPI_ERROR_SETTINGS_CONFLICT;
    }
    g_pwm_config.control_mode = control;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_CONTROL_QUERY(SOURCE_PWM_CONTROL_PWM_CONTROL_t *control) {
    *control = g_pwm_config.control_mode;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_FREQUENCY(float frequency) {
    PWM_REQUIRE_NOT_RUNNING();
    if (frequency * 0.5f < g_pwm_config.phase_speed_hz) {
        return SCPI_ERROR_SETTINGS_CONFLICT;
    }
    g_pwm_config.frequency_hz = frequency;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_FREQUENCY_QUERY(float *frequency) {
    *frequency = g_pwm_config.frequency_hz;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_DEADTIME(float deadtime) {
    // todo: limit deadtime to reasonable range based on frequency and min duty cycle (e.g. not more than 50% of period)
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.deadtime = deadtime;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_DEADTIME_QUERY(float *deadtime) {
    *deadtime = g_pwm_config.deadtime;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_DUTY(const unsigned int indices[1], float duty) {
    unsigned int phase = indices[0];

    g_pwm_config.phase_duty[phase - 1] = duty;
    g_pwm_config.reload_runtime_param = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_DUTY_QUERY(const unsigned int indices[1], float *duty) {
    unsigned int phase = indices[0];

    *duty = g_pwm_config.phase_duty[phase - 1];
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MINDUTY(float min) {
    // todo: limit deadtime to reasonable range based on frequency and min deadtime (e.g. not more than 50% of period)
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.min_duty = min;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MINDUTY_QUERY(float *min) {
    *min = g_pwm_config.min_duty;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MOD(float mod) {
    g_pwm_config.mod_index = mod;
    g_pwm_config.reload_runtime_param = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MOD_QUERY(float *mod) {
    *mod = g_pwm_config.mod_index;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_ANGLE(float angle) {
    /* Normalize angle to [0, 360[ degrees */
    float normalized = fmodf(angle, 360.0f);
    if (normalized < 0.0f) {
        normalized += 360.0f;
    }
    g_pwm_config.phase_angle_deg = normalized;
    g_pwm_config.reload_runtime_param = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_ANGLE_QUERY(float *angle) {
    *angle = g_pwm_config.phase_angle_deg;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_SPEED(float speed) {
    if (speed > (g_pwm_config.frequency_hz * 0.5f)) {
        return SCPI_ERROR_SETTINGS_CONFLICT;
    }
    g_pwm_config.phase_speed_hz = speed;
    g_pwm_config.reload_runtime_param = true;
    return SCPI_ERROR_NO_ERROR;
}
int custom_SOURCE_PWM_SPEED_QUERY(float *speed) {
    *speed = g_pwm_config.phase_speed_hz;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS(const unsigned int indices[1], int gpio) {
    REQUIRE_OUTPUTS_DISABLED();
    PWM_REQUIRE_NOT_RUNNING();
    unsigned int phase = indices[0];
    if (pwm_gpio_in_use(gpio)) {
        if (gpio == g_pwm_config.phase[phase - 1].gpio_ls) { // Allow re-setting the same GPIO without error
            return SCPI_ERROR_NO_ERROR;
        } else {
            return SCPI_ERROR_SETTINGS_CONFLICT; // GPIO is in use by another phase/channel
        }
    }

    g_pwm_config.phase[phase - 1].gpio_ls = gpio;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_QUERY(const unsigned int indices[1], int *gpio) {
    unsigned int phase = indices[0];
    *gpio = g_pwm_config.phase[phase - 1].gpio_ls;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS(const unsigned int indices[1], int gpio) {
    REQUIRE_OUTPUTS_DISABLED();
    PWM_REQUIRE_NOT_RUNNING();
    unsigned int phase = indices[0];
    if (pwm_gpio_in_use(gpio)) {
        if (gpio == g_pwm_config.phase[phase - 1].gpio_hs) { // Allow re-setting the same GPIO without error
            return SCPI_ERROR_NO_ERROR;
        } else {
            return SCPI_ERROR_SETTINGS_CONFLICT; // GPIO is in use by another phase/channel
        }
    }

    g_pwm_config.phase[phase - 1].gpio_hs = gpio;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_QUERY(const unsigned int indices[1], int *gpio) {
    unsigned int phase = indices[0];
    *gpio = g_pwm_config.phase[phase - 1].gpio_hs;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_INVERT(const unsigned int indices[1], bool invert) {
    unsigned int phase = indices[0];
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.phase[phase - 1].ls_inverted = invert;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_INVERT_QUERY(const unsigned int indices[1], bool *invert) {
    unsigned int phase = indices[0];
    *invert = g_pwm_config.phase[phase - 1].ls_inverted;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_INVERT(const unsigned int indices[1], bool invert) {
    unsigned int phase = indices[0];
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.phase[phase - 1].hs_inverted = invert;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_INVERT_QUERY(const unsigned int indices[1], bool *invert) {
    unsigned int phase = indices[0];
    *invert = g_pwm_config.phase[phase - 1].hs_inverted;
    return SCPI_ERROR_NO_ERROR;
}

int custom_OUTPUT_STATE(bool state) {
    g_pwm_config.outputs_enabled = state;
    pwm_gpio_apply_output_state();
    return SCPI_ERROR_NO_ERROR;
}

int custom_OUTPUT_STATE_QUERY(bool *state) {
    *state = g_pwm_config.outputs_enabled;
    return SCPI_ERROR_NO_ERROR;
}

int custom_ABORT(void) {
    g_pwm_config.running = false; /* IRQ and Core1 will handle transition to IDLE */
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_TYPE(SOURCE_BURST_TYPE_BURST_MODE_t type) {
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.burst.type = type;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_TYPE_QUERY(SOURCE_BURST_TYPE_BURST_MODE_t *type) {
    *type = g_pwm_config.burst.type;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_NCYCLES(unsigned int ncycles) {
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.burst.ncycles = ncycles;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_NCYCLES_QUERY(unsigned int *ncycles) {
    *ncycles = g_pwm_config.burst.ncycles;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_DURATION(float duration) {
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.burst.timeout_sec = duration;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_DURATION_QUERY(float *duration) {
    *duration = g_pwm_config.burst.timeout_sec;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_DELAY(float delay) {
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.trigger.trigger_delay_sec = delay;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_DELAY_QUERY(float *delay) {
    *delay = g_pwm_config.trigger.trigger_delay_sec;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_IDLE(const unsigned int indices[1], bool state) {
    unsigned int phase = indices[0];
    PWM_REQUIRE_NOT_RUNNING();

    g_pwm_config.phase[phase - 1].ls_idle = state;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_IDLE_QUERY(const unsigned int indices[1], bool *state) {
    unsigned int phase = indices[0];

    *state = g_pwm_config.phase[phase - 1].ls_idle;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_IDLE(const unsigned int indices[1], bool state) {
    unsigned int phase = indices[0];
    PWM_REQUIRE_NOT_RUNNING();

    g_pwm_config.phase[phase - 1].hs_idle = state;
    g_pwm_config.hardware_config_dirty = true;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_IDLE_QUERY(const unsigned int indices[1], bool *state) {
    unsigned int phase = indices[0];

    *state = g_pwm_config.phase[phase - 1].hs_idle;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_SOURCE(TRIGGER_SOURCE_TRG_SOURCE_t source) {
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.trigger.source = source;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_SOURCE_QUERY(TRIGGER_SOURCE_TRG_SOURCE_t *source) {
    *source = g_pwm_config.trigger.source;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRG(void) {
    if (g_pwm_config.trigger.source == TRG_SOURCE_BUS) {
        g_pwm_config.trigger.bus_triggered = true;
    }
    return SCPI_ERROR_NO_ERROR;
}
