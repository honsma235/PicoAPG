/* Auto-generated SCPI handler stubs and command table
 * Generated from: scpi_commands.yaml
 * DO NOT EDIT - regenerate with: python tools/generate_scpi_docs.py --gen-header
 */
#ifndef SCPI_COMMANDS_GEN_H
#define SCPI_COMMANDS_GEN_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "scpi/scpi.h"

typedef enum {
    MODE_OFF = 0,
    MODE_ONEPH = 1,
    MODE_TWOPH = 2,
    MODE_THREEPH = 3,
} SOURCE_PWM_MODE_MODE_t;

typedef enum {
    CONTROL_DUTY = 0,
    CONTROL_MOD_ANGLE = 1,
    CONTROL_MOD_SPEED = 2,
} SOURCE_PWM_CONTROL_CONTROL_t;

typedef enum {
    ALIGNMENT_CENTER = 0,
    ALIGNMENT_EDGE = 1,
} SOURCE_PWM_ALIGNMENT_ALIGNMENT_t;

/* ============================================================================
 * User-implemented callback functions (weak - provide override)
 * Return value: positive (success) or negative SCPI error code (e.g., SCPI_ERROR_SETTINGS_CONFLICT)
 * ========================================================================== */

/* Event: :SOURce:PWM:MODE */
int custom_SOURCE_PWM_MODE(SOURCE_PWM_MODE_MODE_t mode);
/* Query: :SOURce:PWM:MODE? */
int custom_SOURCE_PWM_MODE_QUERY(SOURCE_PWM_MODE_MODE_t *mode);

/* Event: :SOURce:PWM:CONTrol */
int custom_SOURCE_PWM_CONTROL(SOURCE_PWM_CONTROL_CONTROL_t control);
/* Query: :SOURce:PWM:CONTrol? */
int custom_SOURCE_PWM_CONTROL_QUERY(SOURCE_PWM_CONTROL_CONTROL_t *control);

/* Event: :SOURce:PWM:FREQuency */
int custom_SOURCE_PWM_FREQUENCY(float frequency);
/* Query: :SOURce:PWM:FREQuency? */
int custom_SOURCE_PWM_FREQUENCY_QUERY(float *frequency);

/* Event: :SOURce:PWM:ALIGNment */
int custom_SOURCE_PWM_ALIGNMENT(SOURCE_PWM_ALIGNMENT_ALIGNMENT_t alignment);
/* Query: :SOURce:PWM:ALIGNment? */
int custom_SOURCE_PWM_ALIGNMENT_QUERY(SOURCE_PWM_ALIGNMENT_ALIGNMENT_t *alignment);

/* Event: :SOURce:PWM:DEADtime */
int custom_SOURCE_PWM_DEADTIME(unsigned int deadtime);
/* Query: :SOURce:PWM:DEADtime? */
int custom_SOURCE_PWM_DEADTIME_QUERY(unsigned int *deadtime);

/* Event: :SOURce:PWM:PHase<n>:DUTY */
int custom_SOURCE_PWM_PHASEN_DUTY(const unsigned int indices[1], float duty);
/* Query: :SOURce:PWM:PHase<n>:DUTY? */
int custom_SOURCE_PWM_PHASEN_DUTY_QUERY(const unsigned int indices[1], float *duty);

/* Event: :SOURce:PWM:MINDuty */
int custom_SOURCE_PWM_MINDUTY(float min);
/* Query: :SOURce:PWM:MINDuty? */
int custom_SOURCE_PWM_MINDUTY_QUERY(float *min);

/* Event: :SOURce:PWM:MOD */
int custom_SOURCE_PWM_MOD(float mod);
/* Query: :SOURce:PWM:MOD? */
int custom_SOURCE_PWM_MOD_QUERY(float *mod);

/* Event: :SOURce:PWM:ANGLE */
int custom_SOURCE_PWM_ANGLE(float angle);
/* Query: :SOURce:PWM:ANGLE? */
int custom_SOURCE_PWM_ANGLE_QUERY(float *angle);

/* Event: :SOURce:PWM:SPEED */
int custom_SOURCE_PWM_SPEED(float speed);
/* Query: :SOURce:PWM:SPEED? */
int custom_SOURCE_PWM_SPEED_QUERY(float *speed);

/* Event: :SOURce:PWM:PHase<n>:LS */
int custom_SOURCE_PWM_PHASEN_LS(const unsigned int indices[1], unsigned int gpio);
/* Query: :SOURce:PWM:PHase<n>:LS? */
int custom_SOURCE_PWM_PHASEN_LS_QUERY(const unsigned int indices[1], unsigned int *gpio);

/* Event: :SOURce:PWM:PHase<n>:HS */
int custom_SOURCE_PWM_PHASEN_HS(const unsigned int indices[1], unsigned int gpio);
/* Query: :SOURce:PWM:PHase<n>:HS? */
int custom_SOURCE_PWM_PHASEN_HS_QUERY(const unsigned int indices[1], unsigned int *gpio);

/* Event: :SOURce:PWM:CHANnel:INVert */
int custom_SOURCE_PWM_CHANNEL_INVERT(bool invert);
/* Query: :SOURce:PWM:CHANnel:INVert? */
int custom_SOURCE_PWM_CHANNEL_INVERT_QUERY(bool *invert);

/* ============================================================================
 * Generated command table macro - include in scpi-def.c
 * ========================================================================== */

/* Forward declarations of handlers */
scpi_result_t handler_SOURCE_PWM_MODE(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_MODE_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_CONTROL(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_CONTROL_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_FREQUENCY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_FREQUENCY_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_ALIGNMENT(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_ALIGNMENT_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_DEADTIME(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_DEADTIME_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_PHASEN_DUTY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_PHASEN_DUTY_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_MINDUTY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_MINDUTY_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_MOD(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_MOD_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_ANGLE(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_ANGLE_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_SPEED(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_SPEED_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_PHASEN_LS(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_PHASEN_LS_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_PHASEN_HS(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_PHASEN_HS_QUERY(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_CHANNEL_INVERT(scpi_t *context);
scpi_result_t handler_SOURCE_PWM_CHANNEL_INVERT_QUERY(scpi_t *context);

/* Macro to include all generated commands in scpi_commands[] */
#define SCPI_GENERATED_COMMANDS \
    { .pattern = ":SOURce:PWM:MODE", .callback = handler_SOURCE_PWM_MODE }, \
    { .pattern = ":SOURce:PWM:MODE?", .callback = handler_SOURCE_PWM_MODE_QUERY }, \
    { .pattern = ":SOURce:PWM:CONTrol", .callback = handler_SOURCE_PWM_CONTROL }, \
    { .pattern = ":SOURce:PWM:CONTrol?", .callback = handler_SOURCE_PWM_CONTROL_QUERY }, \
    { .pattern = ":SOURce:PWM:FREQuency", .callback = handler_SOURCE_PWM_FREQUENCY }, \
    { .pattern = ":SOURce:PWM:FREQuency?", .callback = handler_SOURCE_PWM_FREQUENCY_QUERY }, \
    { .pattern = ":SOURce:PWM:ALIGNment", .callback = handler_SOURCE_PWM_ALIGNMENT }, \
    { .pattern = ":SOURce:PWM:ALIGNment?", .callback = handler_SOURCE_PWM_ALIGNMENT_QUERY }, \
    { .pattern = ":SOURce:PWM:DEADtime", .callback = handler_SOURCE_PWM_DEADTIME }, \
    { .pattern = ":SOURce:PWM:DEADtime?", .callback = handler_SOURCE_PWM_DEADTIME_QUERY }, \
    { .pattern = ":SOURce:PWM:PHase#:DUTY", .callback = handler_SOURCE_PWM_PHASEN_DUTY }, \
    { .pattern = ":SOURce:PWM:PHase#:DUTY?", .callback = handler_SOURCE_PWM_PHASEN_DUTY_QUERY }, \
    { .pattern = ":SOURce:PWM:MINDuty", .callback = handler_SOURCE_PWM_MINDUTY }, \
    { .pattern = ":SOURce:PWM:MINDuty?", .callback = handler_SOURCE_PWM_MINDUTY_QUERY }, \
    { .pattern = ":SOURce:PWM:MOD", .callback = handler_SOURCE_PWM_MOD }, \
    { .pattern = ":SOURce:PWM:MOD?", .callback = handler_SOURCE_PWM_MOD_QUERY }, \
    { .pattern = ":SOURce:PWM:ANGLE", .callback = handler_SOURCE_PWM_ANGLE }, \
    { .pattern = ":SOURce:PWM:ANGLE?", .callback = handler_SOURCE_PWM_ANGLE_QUERY }, \
    { .pattern = ":SOURce:PWM:SPEED", .callback = handler_SOURCE_PWM_SPEED }, \
    { .pattern = ":SOURce:PWM:SPEED?", .callback = handler_SOURCE_PWM_SPEED_QUERY }, \
    { .pattern = ":SOURce:PWM:PHase#:LS", .callback = handler_SOURCE_PWM_PHASEN_LS }, \
    { .pattern = ":SOURce:PWM:PHase#:LS?", .callback = handler_SOURCE_PWM_PHASEN_LS_QUERY }, \
    { .pattern = ":SOURce:PWM:PHase#:HS", .callback = handler_SOURCE_PWM_PHASEN_HS }, \
    { .pattern = ":SOURce:PWM:PHase#:HS?", .callback = handler_SOURCE_PWM_PHASEN_HS_QUERY }, \
    { .pattern = ":SOURce:PWM:CHANnel:INVert", .callback = handler_SOURCE_PWM_CHANNEL_INVERT }, \
    { .pattern = ":SOURce:PWM:CHANnel:INVert?", .callback = handler_SOURCE_PWM_CHANNEL_INVERT_QUERY }, \
    /* End of generated commands */

#endif /* SCPI_COMMANDS_GEN_H */