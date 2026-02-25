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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apg/apg.h"
#include "common/main_core1.h"
#include "common/output.h"
#include "common/trigger.h"
#include "pwm/pwm.h"
#include "pwm/pwm_gpio.h"
#include "scpi_commands_gen.h"

/* Helper macros for common checks */
#define REQUIRE_OUTPUTS_DISABLED()               \
    do {                                         \
        if (g_output_state.enabled) {            \
            return SCPI_ERROR_SETTINGS_CONFLICT; \
        }                                        \
    } while (0)

#define PWM_REQUIRE_NOT_RUNNING()                      \
    do {                                               \
        if (g_pwm_config.state == PWM_STATE_RUNNING) { \
            return SCPI_ERROR_SETTINGS_CONFLICT;       \
        }                                              \
    } while (0)

int custom_OUTPUT_STATE(bool state) {
    g_output_state.enabled = state;
    output_apply_state();
    return SCPI_ERROR_NO_ERROR;
}

int custom_OUTPUT_STATE_QUERY(bool *state) {
    *state = g_output_state.enabled;
    return SCPI_ERROR_NO_ERROR;
}

int custom_ABORT(void) {
    abort_all();
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_SOURCE(TRIGGER_SOURCE_TRG_SOURCE_t source) {
    g_trigger_config.source = source;
    trigger_update_config();
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_SOURCE_QUERY(TRIGGER_SOURCE_TRG_SOURCE_t *source) {
    *source = g_trigger_config.source;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_DELAY(float delay) {
    g_trigger_config.delay_sec = delay;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRIGGER_DELAY_QUERY(float *delay) {
    *delay = g_trigger_config.delay_sec;
    return SCPI_ERROR_NO_ERROR;
}

int custom_TRG(void) {
    trigger_fire(TRG_SOURCE_BUS);
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_TYPE(SOURCE_BURST_TYPE_BURST_MODE_t type) {
    abort_all(); /* Ensure no triggers are pending/running with old burst type */
    g_trigger_config.burst_type = type;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_TYPE_QUERY(SOURCE_BURST_TYPE_BURST_MODE_t *type) {
    *type = g_trigger_config.burst_type;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_NCYCLES(unsigned int ncycles) {
    g_trigger_config.burst_ncycles = ncycles;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_NCYCLES_QUERY(unsigned int *ncycles) {
    *ncycles = g_trigger_config.burst_ncycles;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_DURATION(float duration) {
    g_trigger_config.burst_duration_sec = duration;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_DURATION_QUERY(float *duration) {
    *duration = g_trigger_config.burst_duration_sec;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_INTERVAL(float interval) {
    g_trigger_config.interval_sec = interval;
    trigger_update_config();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_INTERVAL_QUERY(float *interval) {
    *interval = g_trigger_config.interval_sec;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_FREQUENCY(float frequency) {
    g_trigger_config.interval_sec = 1.0f / frequency;
    trigger_update_config();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_BURST_FREQUENCY_QUERY(float *frequency) {
    *frequency = 1.0f / g_trigger_config.interval_sec;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MODE(SOURCE_PWM_MODE_PWM_MODE_t mode) {
    REQUIRE_OUTPUTS_DISABLED();
    if (mode == PWM_MODE_ONEPH && g_pwm_config.control_mode != PWM_CONTROL_DUTY) {
        // Only DUTY control is valid in ONEPH mode
        return SCPI_ERROR_SETTINGS_CONFLICT;
    }
    pwm_abort(); /* Ensure PWM is stopped before changing mode */
    g_pwm_config.op_mode = mode;
    pwm_update_config();
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
    pwm_update_config();
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
    pwm_update_config();
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
    pwm_update_config();
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

int custom_SOURCE_PWM_PHASEN_LS_GPIO(const unsigned int indices[1], int gpio) {
    REQUIRE_OUTPUTS_DISABLED();
    unsigned int phase = indices[0];
    if (apg_gpio_in_use(-1, gpio)) {
        return SCPI_ERROR_SETTINGS_CONFLICT; // GPIO is in use by APG
    }
    if (pwm_gpio_in_use(gpio)) {
        if (gpio == g_pwm_config.phase[phase - 1].gpio_ls) { // Allow re-setting the same GPIO without error
            return SCPI_ERROR_NO_ERROR;
        } else {
            return SCPI_ERROR_SETTINGS_CONFLICT; // GPIO is in use by another phase/channel
        }
    }
    pwm_abort(); /* Ensure PWM is stopped before changing GPIO config */
    g_pwm_config.phase[phase - 1].gpio_ls = gpio;
    pwm_update_config();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_GPIO_QUERY(const unsigned int indices[1], int *gpio) {
    unsigned int phase = indices[0];
    *gpio = g_pwm_config.phase[phase - 1].gpio_ls;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_GPIO(const unsigned int indices[1], int gpio) {
    REQUIRE_OUTPUTS_DISABLED();
    unsigned int phase = indices[0];
    if (apg_gpio_in_use(-1, gpio)) {
        return SCPI_ERROR_SETTINGS_CONFLICT; // GPIO is in use by APG
    }
    if (pwm_gpio_in_use(gpio)) {
        if (gpio == g_pwm_config.phase[phase - 1].gpio_hs) { // Allow re-setting the same GPIO without error
            return SCPI_ERROR_NO_ERROR;
        } else {
            return SCPI_ERROR_SETTINGS_CONFLICT; // GPIO is in use by another phase/channel
        }
    }
    pwm_abort(); /* Ensure PWM is stopped before changing GPIO config */
    g_pwm_config.phase[phase - 1].gpio_hs = gpio;
    pwm_update_config();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_GPIO_QUERY(const unsigned int indices[1], int *gpio) {
    unsigned int phase = indices[0];
    *gpio = g_pwm_config.phase[phase - 1].gpio_hs;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_INVERT(const unsigned int indices[1], bool invert) {
    unsigned int phase = indices[0];
    PWM_REQUIRE_NOT_RUNNING();
    g_pwm_config.phase[phase - 1].ls_inverted = invert;
    pwm_update_config();
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
    pwm_update_config();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_INVERT_QUERY(const unsigned int indices[1], bool *invert) {
    unsigned int phase = indices[0];
    *invert = g_pwm_config.phase[phase - 1].hs_inverted;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_IDLE(const unsigned int indices[1], bool state) {
    unsigned int phase = indices[0];
    PWM_REQUIRE_NOT_RUNNING();

    g_pwm_config.phase[phase - 1].ls_idle = state;
    pwm_update_config();
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
    pwm_update_config();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_IDLE_QUERY(const unsigned int indices[1], bool *state) {
    unsigned int phase = indices[0];

    *state = g_pwm_config.phase[phase - 1].hs_idle;
    return SCPI_ERROR_NO_ERROR;
}

/**
 * APG command implementations
 */

int custom_SOURCE_APG_STATE(bool state) {
    apg_set_state(state);
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_APG_STATE_QUERY(bool *state) {
    *state = g_apg_is_enabled;
    return SCPI_ERROR_NO_ERROR;
}

static scpi_result_t parse_apg_pairs(scpi_t *context, apg_value_duration_t **out_pairs, size_t *out_count) {
    if (out_pairs == NULL || out_count == NULL) {
        return SCPI_RES_ERR;
    }
    if (*out_pairs != NULL) {
        return SCPI_RES_ERR;
    }

    *out_count = 0;

    size_t count = 0;
    size_t capacity = 0;
    scpi_parameter_t param;

    for (;;) {
        uint32_t value = 0;
        if (!SCPI_ParamUInt32(context, &value, count == 0 ? TRUE : FALSE)) {
            if (SCPI_ParamErrorOccurred(context) || (count == 0)) {
                /* something wrong with the parameter or missing mandatory first parameter */
                return SCPI_RES_ERR;
            } else {
                /* no more parameters */
                break;
            }
        }

        // Value is 24-bit unsigned, so max is 0x00FFFFFF
        if (value > 0x00FFFFFF) {
            SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);
            return SCPI_RES_ERR;
        }

        double duration = 0.0;
        if (!SCPI_ParamDouble(context, &duration, TRUE)) {
            return SCPI_RES_ERR;
        }
        // Minimum duration is 20 ns, maximum is 60 s
        if (duration < 20.0e-9 || duration > 60.0) {
            SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);
            return SCPI_RES_ERR;
        }

        if (count == capacity) {
            size_t next_capacity = capacity + 8u;
            apg_value_duration_t *next_pairs = realloc(*out_pairs, next_capacity * sizeof(apg_value_duration_t));
            if (!next_pairs) {
                SCPI_ErrorPush(context, SCPI_ERROR_OUT_OF_MEMORY);
                return SCPI_RES_ERR;
            }
            *out_pairs = next_pairs;
            capacity = next_capacity;
        }

        (*out_pairs)[count].value = value;
        (*out_pairs)[count].duration_sec = duration;
        count++;
    }

    *out_count = count;
    return SCPI_RES_OK;
}

scpi_result_t custom_SOURCE_APG_DATA(scpi_t *context) {
    apg_value_duration_t *pairs = NULL;
    size_t count = 0;

    if (g_output_state.enabled) {
        SCPI_ErrorPush(context, SCPI_ERROR_SETTINGS_CONFLICT);
        return SCPI_RES_ERR;
    }

    if (parse_apg_pairs(context, &pairs, &count) != SCPI_RES_OK) {
        free(pairs);
        return SCPI_RES_ERR;
    }

    if (apg_write_data(pairs, count, false) != 0) {
        free(pairs);
        SCPI_ErrorPush(context, SCPI_ERROR_TOO_MUCH_DATA);
        return SCPI_RES_ERR;
    }

    free(pairs);
    return SCPI_RES_OK;
}

scpi_result_t custom_SOURCE_APG_DATA_QUERY(scpi_t *context) {
    apg_value_duration_t *pairs = NULL;
    size_t count = 0;
    if (apg_read_data(&pairs, &count) != 0) {
        free(pairs);
        SCPI_ErrorPush(context, SCPI_ERROR_OUT_OF_MEMORY);
        return SCPI_RES_ERR;
    }

    for (size_t i = 0; i < count; i++) {
        SCPI_ResultUInt32(context, pairs[i].value);
        SCPI_ResultDouble(context, pairs[i].duration_sec);
    }

    free(pairs);
    return SCPI_RES_OK;
}

scpi_result_t custom_SOURCE_APG_DATA_APPEND(scpi_t *context) {
    apg_value_duration_t *pairs = NULL;
    size_t count = 0;

    if (g_output_state.enabled) {
        SCPI_ErrorPush(context, SCPI_ERROR_SETTINGS_CONFLICT);
        return SCPI_RES_ERR;
    }

    if (parse_apg_pairs(context, &pairs, &count) != SCPI_RES_OK) {
        free(pairs);
        return SCPI_RES_ERR;
    }

    if (apg_write_data(pairs, count, true) != 0) {
        free(pairs);
        SCPI_ErrorPush(context, SCPI_ERROR_TOO_MUCH_DATA);
        return SCPI_RES_ERR;
    }

    free(pairs);
    return SCPI_RES_OK;
}

scpi_result_t custom_SOURCE_APG_DATA_POINTS(scpi_t *context) {
    SCPI_ResultUInt32(context, (uint32_t)g_apg_data_count);
    return SCPI_RES_OK;
}

int custom_SOURCE_APG_IDLE_MODE(SOURCE_APG_IDLE_MODE_IDLE_MODE_t idle_mode) {
    g_apg_idle_mode = idle_mode;
    apg_update_idle();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_APG_IDLE_MODE_QUERY(SOURCE_APG_IDLE_MODE_IDLE_MODE_t *idle_mode) {
    *idle_mode = g_apg_idle_mode;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_APG_IDLE_VALUE(unsigned int idle_value) {
    g_apg_idle_value = idle_value;
    apg_update_idle();
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_APG_IDLE_VALUE_QUERY(unsigned int *idle_value) {
    *idle_value = g_apg_idle_value;
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_APG_MAP_BITN_GPIO(const unsigned int indices[1], int gpio) {
    REQUIRE_OUTPUTS_DISABLED();

    if (pwm_gpio_in_use(gpio) || apg_gpio_in_use(indices[0], gpio)) {
        return SCPI_ERROR_SETTINGS_CONFLICT;
    }

    apg_set_mapping(indices[0], gpio);

    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_APG_MAP_BITN_GPIO_QUERY(const unsigned int indices[1], int *gpio) {
    apg_get_mapping(indices[0], gpio);
    return SCPI_ERROR_NO_ERROR;
}
