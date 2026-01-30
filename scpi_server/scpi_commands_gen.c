/* Auto-generated SCPI handlers
 * Generated from: scpi_commands.yaml
 * DO NOT EDIT - regenerate with: python tools/generate_scpi_docs.py --gen-header
 */
#include "scpi_commands_gen.h"
#include "scpi-def.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* ============================================================================
 * Generated helper functions
 * ========================================================================== */

static inline scpi_result_t scpi_gen_parse_indices(scpi_t *context, unsigned int *indices, const unsigned int *min_vals, const unsigned int *max_vals, size_t count) {
    if (!SCPI_CommandNumbers(context, (int32_t*)indices, count, -1)) {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
        return SCPI_RES_ERR;
    }
    for (size_t i = 0; i < count; ++i) {
        if (indices[i] < min_vals[i] || indices[i] > max_vals[i]) {
            SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
            return SCPI_RES_ERR;
        }
    }
    return SCPI_RES_OK;
}

static inline scpi_result_t scpi_gen_parse_int(scpi_t *context, int *out, bool has_min, int min_val, bool has_max, int max_val) {
    int32_t value;
    if (!SCPI_ParamInt32(context, &value, TRUE)) {
        return SCPI_RES_ERR;
    }
    if ((has_min && value < min_val) || (has_max && value > max_val)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }
    *out = (int)value;
    return SCPI_RES_OK;
}

static inline scpi_result_t scpi_gen_parse_uint(scpi_t *context, unsigned int *out, bool has_min, unsigned int min_val, bool has_max, unsigned int max_val) {
    uint32_t value;
    if (!SCPI_ParamUInt32(context, &value, TRUE)) {
        return SCPI_RES_ERR;
    }
    if ((has_min && value < min_val) || (has_max && value > max_val)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }
    *out = (unsigned int)value;
    return SCPI_RES_OK;
}

static inline scpi_result_t scpi_gen_parse_float(scpi_t *context, float *out, bool has_min, float min_val, bool has_max, float max_val) {
    float value;
    if (!SCPI_ParamFloat(context, &value, TRUE)) {
        return SCPI_RES_ERR;
    }
    if ((has_min && value < min_val) || (has_max && value > max_val)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }
    *out = value;
    return SCPI_RES_OK;
}

static inline scpi_result_t scpi_gen_parse_bool(scpi_t *context, bool *out) {
    scpi_bool_t value;
    if (!SCPI_ParamBool(context, &value, TRUE)) {
        return SCPI_RES_ERR;
    }
    *out = value ? true : false;
    return SCPI_RES_OK;
}

static inline void scpi_gen_result_int(scpi_t *context, int value) {
    SCPI_ResultInt32(context, (int32_t)value);
}

static inline void scpi_gen_result_uint(scpi_t *context, unsigned int value) {
    SCPI_ResultUInt32(context, (uint32_t)value);
}

static inline void scpi_gen_result_float(scpi_t *context, float value) {
    SCPI_ResultFloat(context, value);
}

/* ============================================================================
 * Choice lists for enum parameters
 * ========================================================================== */

static const scpi_choice_def_t source_MODE_choices[] = {
    {"OFF", MODE_OFF},
    {"ONEPH", MODE_ONEPH},
    {"TWOPH", MODE_TWOPH},
    {"THREEPH", MODE_THREEPH},
    SCPI_CHOICE_LIST_END
};

static const scpi_choice_def_t source_CONTROL_choices[] = {
    {"DUTY", CONTROL_DUTY},
    {"MOD_ANGLE", CONTROL_MOD_ANGLE},
    {"MOD_SPEED", CONTROL_MOD_SPEED},
    SCPI_CHOICE_LIST_END
};

static const scpi_choice_def_t source_ALIGNMENT_choices[] = {
    {"CENTER", ALIGNMENT_CENTER},
    {"EDGE", ALIGNMENT_EDGE},
    SCPI_CHOICE_LIST_END
};

/* ============================================================================
 * Weak stub implementations (override in scpi_commands.c)
 * ========================================================================== */

__attribute__((weak))
int custom_SOURCE_PWM_MODE(SOURCE_PWM_MODE_MODE_t mode) {
    (void)mode;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_MODE_QUERY(SOURCE_PWM_MODE_MODE_t *mode) {
    (void)mode;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_CONTROL(SOURCE_PWM_CONTROL_CONTROL_t control) {
    (void)control;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_CONTROL_QUERY(SOURCE_PWM_CONTROL_CONTROL_t *control) {
    (void)control;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_FREQUENCY(float frequency) {
    (void)frequency;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_FREQUENCY_QUERY(float *frequency) {
    (void)frequency;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_ALIGNMENT(SOURCE_PWM_ALIGNMENT_ALIGNMENT_t alignment) {
    (void)alignment;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_ALIGNMENT_QUERY(SOURCE_PWM_ALIGNMENT_ALIGNMENT_t *alignment) {
    (void)alignment;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_DEADTIME(unsigned int deadtime) {
    (void)deadtime;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_DEADTIME_QUERY(unsigned int *deadtime) {
    (void)deadtime;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_PHASEN_DUTY(const unsigned int indices[1], float duty) {
    (void)indices;
    (void)duty;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_PHASEN_DUTY_QUERY(const unsigned int indices[1], float *duty) {
    (void)indices;
    (void)duty;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_MINDUTY(float min) {
    (void)min;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_MINDUTY_QUERY(float *min) {
    (void)min;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_MOD(float mod) {
    (void)mod;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_MOD_QUERY(float *mod) {
    (void)mod;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_ANGLE(float angle) {
    (void)angle;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_ANGLE_QUERY(float *angle) {
    (void)angle;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_SPEED(float speed) {
    (void)speed;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_SPEED_QUERY(float *speed) {
    (void)speed;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_PHASEN_LS(const unsigned int indices[1], unsigned int gpio) {
    (void)indices;
    (void)gpio;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_PHASEN_LS_QUERY(const unsigned int indices[1], unsigned int *gpio) {
    (void)indices;
    (void)gpio;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_PHASEN_HS(const unsigned int indices[1], unsigned int gpio) {
    (void)indices;
    (void)gpio;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_PHASEN_HS_QUERY(const unsigned int indices[1], unsigned int *gpio) {
    (void)indices;
    (void)gpio;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_CHANNEL_INVERT(bool invert) {
    (void)invert;
    return 0; /* Not implemented */
}

__attribute__((weak))
int custom_SOURCE_PWM_CHANNEL_INVERT_QUERY(bool *invert) {
    (void)invert;
    return 0; /* Not implemented */
}

/* ============================================================================
 * SCPI Command Handlers (parse, validate, call custom hooks)
 * ========================================================================== */

scpi_result_t handler_SOURCE_PWM_MODE(scpi_t *context) {
    SOURCE_PWM_MODE_MODE_t mode = 0;
    int result;

    /* Parse and validate parameters */
    /* Parse mode (enum) */
    {
        int32_t mode_value = 0;
        if (!SCPI_ParamChoice(context, source_MODE_choices, &mode_value, TRUE)) {
            return SCPI_RES_ERR;
        }
        mode = (SOURCE_PWM_MODE_MODE_t)mode_value;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_MODE(mode);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_MODE_QUERY(scpi_t *context) {
    SOURCE_PWM_MODE_MODE_t mode = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_MODE_QUERY(&mode);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    /* Format enum as string */
    {
        const char *name;
        if (SCPI_ChoiceToName(source_MODE_choices, (int32_t)mode, &name)) {
            SCPI_ResultMnemonic(context, name);
        }
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_CONTROL(scpi_t *context) {
    SOURCE_PWM_CONTROL_CONTROL_t control = 0;
    int result;

    /* Parse and validate parameters */
    /* Parse control (enum) */
    {
        int32_t control_value = 0;
        if (!SCPI_ParamChoice(context, source_CONTROL_choices, &control_value, TRUE)) {
            return SCPI_RES_ERR;
        }
        control = (SOURCE_PWM_CONTROL_CONTROL_t)control_value;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_CONTROL(control);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_CONTROL_QUERY(scpi_t *context) {
    SOURCE_PWM_CONTROL_CONTROL_t control = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_CONTROL_QUERY(&control);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    /* Format enum as string */
    {
        const char *name;
        if (SCPI_ChoiceToName(source_CONTROL_choices, (int32_t)control, &name)) {
            SCPI_ResultMnemonic(context, name);
        }
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_FREQUENCY(scpi_t *context) {
    float frequency = 0;
    int result;

    /* Parse and validate parameters */
    if (scpi_gen_parse_float(context, &frequency, true, 1.0, true, 100000.0) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_FREQUENCY(frequency);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_FREQUENCY_QUERY(scpi_t *context) {
    float frequency = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_FREQUENCY_QUERY(&frequency);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_float(context, frequency);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_ALIGNMENT(scpi_t *context) {
    SOURCE_PWM_ALIGNMENT_ALIGNMENT_t alignment = 0;
    int result;

    /* Parse and validate parameters */
    /* Parse alignment (enum) */
    {
        int32_t alignment_value = 0;
        if (!SCPI_ParamChoice(context, source_ALIGNMENT_choices, &alignment_value, TRUE)) {
            return SCPI_RES_ERR;
        }
        alignment = (SOURCE_PWM_ALIGNMENT_ALIGNMENT_t)alignment_value;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_ALIGNMENT(alignment);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_ALIGNMENT_QUERY(scpi_t *context) {
    SOURCE_PWM_ALIGNMENT_ALIGNMENT_t alignment = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_ALIGNMENT_QUERY(&alignment);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    /* Format enum as string */
    {
        const char *name;
        if (SCPI_ChoiceToName(source_ALIGNMENT_choices, (int32_t)alignment, &name)) {
            SCPI_ResultMnemonic(context, name);
        }
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_DEADTIME(scpi_t *context) {
    unsigned int deadtime = 0;
    int result;

    /* Parse and validate parameters */
    if (scpi_gen_parse_uint(context, &deadtime, true, 0, true, 10000) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_DEADTIME(deadtime);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_DEADTIME_QUERY(scpi_t *context) {
    unsigned int deadtime = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_DEADTIME_QUERY(&deadtime);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_uint(context, deadtime);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_PHASEN_DUTY(scpi_t *context) {
    float duty = 0;
    unsigned int indices[1] = {0};
    int result;

    /* Extract and validate indices */
    if (scpi_gen_parse_indices(context, indices, (const unsigned int[]){1}, (const unsigned int[]){3}, 1) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Parse and validate parameters */
    if (scpi_gen_parse_float(context, &duty, true, 0.0, true, 1.0) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_PHASEN_DUTY(indices, duty);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_PHASEN_DUTY_QUERY(scpi_t *context) {
    float duty = 0;
    unsigned int indices[1] = {0};
    int result;

    /* Extract and validate indices */
    if (scpi_gen_parse_indices(context, indices, (const unsigned int[]){1}, (const unsigned int[]){3}, 1) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_PHASEN_DUTY_QUERY(indices, &duty);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_float(context, duty);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_MINDUTY(scpi_t *context) {
    float min = 0;
    int result;

    /* Parse and validate parameters */
    if (scpi_gen_parse_float(context, &min, true, 0.0, true, 0.2) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_MINDUTY(min);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_MINDUTY_QUERY(scpi_t *context) {
    float min = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_MINDUTY_QUERY(&min);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_float(context, min);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_MOD(scpi_t *context) {
    float mod = 0;
    int result;

    /* Parse and validate parameters */
    if (scpi_gen_parse_float(context, &mod, true, 0.0, true, 1.0) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_MOD(mod);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_MOD_QUERY(scpi_t *context) {
    float mod = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_MOD_QUERY(&mod);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_float(context, mod);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_ANGLE(scpi_t *context) {
    float angle = 0;
    int result;

    /* Parse and validate parameters */
    if (scpi_gen_parse_float(context, &angle, false, 0, false, 0) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_ANGLE(angle);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_ANGLE_QUERY(scpi_t *context) {
    float angle = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_ANGLE_QUERY(&angle);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_float(context, angle);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_SPEED(scpi_t *context) {
    float speed = 0;
    int result;

    /* Parse and validate parameters */
    if (scpi_gen_parse_float(context, &speed, true, 0, true, 100000) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_SPEED(speed);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_SPEED_QUERY(scpi_t *context) {
    float speed = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_SPEED_QUERY(&speed);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_float(context, speed);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_PHASEN_LS(scpi_t *context) {
    unsigned int gpio = 0;
    unsigned int indices[1] = {0};
    int result;

    /* Extract and validate indices */
    if (scpi_gen_parse_indices(context, indices, (const unsigned int[]){1}, (const unsigned int[]){3}, 1) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Parse and validate parameters */
    if (scpi_gen_parse_uint(context, &gpio, true, 0, true, 29) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_PHASEN_LS(indices, gpio);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_PHASEN_LS_QUERY(scpi_t *context) {
    unsigned int gpio = 0;
    unsigned int indices[1] = {0};
    int result;

    /* Extract and validate indices */
    if (scpi_gen_parse_indices(context, indices, (const unsigned int[]){1}, (const unsigned int[]){3}, 1) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_PHASEN_LS_QUERY(indices, &gpio);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_uint(context, gpio);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_PHASEN_HS(scpi_t *context) {
    unsigned int gpio = 0;
    unsigned int indices[1] = {0};
    int result;

    /* Extract and validate indices */
    if (scpi_gen_parse_indices(context, indices, (const unsigned int[]){1}, (const unsigned int[]){3}, 1) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Parse and validate parameters */
    if (scpi_gen_parse_uint(context, &gpio, true, 0, true, 29) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_PHASEN_HS(indices, gpio);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_PHASEN_HS_QUERY(scpi_t *context) {
    unsigned int gpio = 0;
    unsigned int indices[1] = {0};
    int result;

    /* Extract and validate indices */
    if (scpi_gen_parse_indices(context, indices, (const unsigned int[]){1}, (const unsigned int[]){3}, 1) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_PHASEN_HS_QUERY(indices, &gpio);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    scpi_gen_result_uint(context, gpio);

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_CHANNEL_INVERT(scpi_t *context) {
    bool invert = 0;
    int result;

    /* Parse and validate parameters */
    if (scpi_gen_parse_bool(context, &invert) != SCPI_RES_OK) {
        return SCPI_RES_ERR;
    }

    /* Call custom implementation */
    result = custom_SOURCE_PWM_CHANNEL_INVERT(invert);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t handler_SOURCE_PWM_CHANNEL_INVERT_QUERY(scpi_t *context) {
    bool invert = 0;
    int result;

    /* Call custom query implementation */
    result = custom_SOURCE_PWM_CHANNEL_INVERT_QUERY(&invert);
    if (result < 0) {
        SCPI_ErrorPush(context, result);
        return SCPI_RES_ERR;
    }

    /* Format response based on parameter types */
    SCPI_ResultBool(context, invert);

    return SCPI_RES_OK;
}
