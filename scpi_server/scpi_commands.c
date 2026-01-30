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

#include "scpi_commands_gen.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>


/* ============================================================================
 * Example Implementation: PWM Mode (SET/GET)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:Mode <mode>
 *          :SOURce:PWM:Mode?
 * 
 * This example shows how to implement a simple SET/GET pair.
 */

/* Local state storage for PWM mode */
static SOURCE_PWM_MODE_MODE_t pwm_mode = MODE_OFF;

/* Custom Event hook for PWM mode */
int custom_SOURCE_PWM_MODE(SOURCE_PWM_MODE_MODE_t mode) {
    /* Store the mode */
    pwm_mode = mode;
    
    /* TODO: Actually apply the mode to hardware*/
    
    if (mode == MODE_ONEPH) {
        return SCPI_ERROR_SETTINGS_CONFLICT;  /* Example error: ONEPH mode not supported */
    }
    return SCPI_ERROR_NO_ERROR;
}

/* Custom Query hook for PWM mode */
int custom_SOURCE_PWM_MODE_QUERY(SOURCE_PWM_MODE_MODE_t *mode) {
    /* Fill in the response with current mode */
    *mode = pwm_mode;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: PWM Frequency (SET/GET with range validation)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:FREQuency <frequency>
 *          :SOURce:PWM:FREQuency?
 * 
 * This example demonstrates parameter validation and range checking.
 * The YAML defines: min=1.0, max=100000.0 Hz
 */

static float pwm_frequency = 10000.0f;

int custom_SOURCE_PWM_FREQUENCY(float frequency) {
    pwm_frequency = frequency;
    
    /* TODO: Configure hardware PWM carrier frequency */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_FREQUENCY_QUERY(float *frequency) {
    *frequency = pwm_frequency;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: Indexed Command (PWM Phase Duty Cycle)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:PHase<n>:DUTY <duty>
 *          :SOURce:PWM:PHase<n>:DUTY?
 * 
 * Where <n> is 1-3 (indexed).
 */

static float pwm_duty[4] = {0.0f, 0.5f, 0.5f, 0.5f};  /* Index 0 unused, 1-3 for phases */

int custom_SOURCE_PWM_PHASEN_DUTY(const unsigned int indices[1], float duty) {
    /* Extract phase index */
    unsigned int phase = indices[0];  /* phase will be 1, 2, or 3 */
    
    pwm_duty[phase] = duty;
    
    /* TODO: Configure hardware with new duty cycle */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_DUTY_QUERY(const unsigned int indices[1], float *duty) {
    unsigned int phase = indices[0];
    
    *duty = pwm_duty[phase];
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: PWM Alignment (Enum with two values)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:ALIGNment <alignment>
 *          :SOURce:PWM:ALIGNment?
 * 
 * This example shows enum type with values: CENTER, EDGE
 */

static SOURCE_PWM_ALIGNMENT_ALIGNMENT_t pwm_alignment = ALIGNMENT_CENTER;

int custom_SOURCE_PWM_ALIGNMENT(SOURCE_PWM_ALIGNMENT_ALIGNMENT_t alignment) {
    pwm_alignment = alignment;
    
    /* TODO: Configure PWM alignment on hardware */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_ALIGNMENT_QUERY(SOURCE_PWM_ALIGNMENT_ALIGNMENT_t *alignment) {
    *alignment = pwm_alignment;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: PWM Deadtime (Unsigned Int with range)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:DEADtime <deadtime>
 *          :SOURce:PWM:DEADtime?
 * 
 * This example demonstrates uint type. Range: 0-10000 ns
 */

static unsigned int pwm_deadtime = 1000;

int custom_SOURCE_PWM_DEADTIME(unsigned int deadtime) {
    pwm_deadtime = deadtime;
    
    /* TODO: Configure deadtime on hardware */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_DEADTIME_QUERY(unsigned int *deadtime) {
    *deadtime = pwm_deadtime;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: PWM Control Mode (Enum with three values)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:CONTrol <control>
 *          :SOURce:PWM:CONTrol?
 * 
 * This example shows enum type with three values: DUTY, MI_PHASE, MI_SPEED
 */

static SOURCE_PWM_CONTROL_CONTROL_t pwm_control = CONTROL_DUTY;

int custom_SOURCE_PWM_CONTROL(SOURCE_PWM_CONTROL_CONTROL_t control) {
    pwm_control = control;
    
    /* TODO: Switch control mode */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_CONTROL_QUERY(SOURCE_PWM_CONTROL_CONTROL_t *control) {
    *control = pwm_control;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: Min Duty Cycle (Float with range 0.0-0.2)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:MINDuty <min>
 *          :SOURce:PWM:MINDuty?
 */

static float pwm_min_duty = 0.05f;

int custom_SOURCE_PWM_MINDUTY(float min) {
    pwm_min_duty = min;
    
    /* TODO: Apply minimum duty cycle constraint */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MINDUTY_QUERY(float *min) {
    *min = pwm_min_duty;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: Modulation Index (Float 0.0-1.0)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:MOD <mod>
 *          :SOURce:PWM:MOD?
 */

static float pwm_modulation_index = 0.5f;

int custom_SOURCE_PWM_MOD(float mod) {
    pwm_modulation_index = mod;
    
    /* TODO: Update SPWM modulation index */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_MOD_QUERY(float *mod) {
    *mod = pwm_modulation_index;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: PWM Angle (Float)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:ANGLE <angle>
 *          :SOURce:PWM:ANGLE?
 */

static float pwm_angle = 0.0f;

int custom_SOURCE_PWM_ANGLE(float angle) {
    pwm_angle = angle;
    
    /* TODO: Update SPWM phase angle */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_ANGLE_QUERY(float *angle) {
    *angle = pwm_angle;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: PWM Rotation Speed (Float in RPM)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:SPEED <speed>
 *          :SOURce:PWM:SPEED?
 */

static float pwm_rotation_speed = 100.0f;

int custom_SOURCE_PWM_SPEED(float speed) {
    pwm_rotation_speed = speed;
    
    /* TODO: Update SPWM rotation speed */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_SPEED_QUERY(float *speed) {
    *speed = pwm_rotation_speed;
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: GPIO Assignments (Unsigned Int, Indexed)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:PHase<n>:LS <gpio>
 *          :SOURce:PWM:PHase<n>:LS?
 * 
 * Command: :SOURce:PWM:PHase<n>:HS <gpio>
 *          :SOURce:PWM:PHase<n>:HS?
 */

static unsigned int pwm_ls_gpio[4] = {0, 0, 1, 2};    /* Low-side GPIO assignments */
static unsigned int pwm_hs_gpio[4] = {0, 3, 4, 5};    /* High-side GPIO assignments */

int custom_SOURCE_PWM_PHASEN_LS(const unsigned int indices[1], unsigned int gpio) {
    unsigned int phase = indices[0];  /* phase 1, 2, or 3 */
    pwm_ls_gpio[phase] = gpio;
    
    /* TODO: Configure GPIO assignments on hardware */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_LS_QUERY(const unsigned int indices[1], unsigned int *gpio) {
    unsigned int phase = indices[0];
    *gpio = pwm_ls_gpio[phase];
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS(const unsigned int indices[1], unsigned int gpio) {
    unsigned int phase = indices[0];  /* phase 1, 2, or 3 */
    pwm_hs_gpio[phase] = gpio;
    
    /* TODO: Configure GPIO assignments on hardware */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_PHASEN_HS_QUERY(const unsigned int indices[1], unsigned int *gpio) {
    unsigned int phase = indices[0];
    *gpio = pwm_hs_gpio[phase];
    return SCPI_ERROR_NO_ERROR;
}


/* ============================================================================
 * Example Implementation: Channel Invert (Boolean)
 * ============================================================================
 * 
 * Command: :SOURce:PWM:CHANnel:INVert <invert>
 *          :SOURce:PWM:CHANnel:INVert?
 * 
 * This example shows bool type. Set to true/false or ON/OFF
 */

static bool pwm_channel_invert = false;

int custom_SOURCE_PWM_CHANNEL_INVERT(bool invert) {
    pwm_channel_invert = invert;
    
    /* TODO: Configure channel output inversion on hardware */
    
    return SCPI_ERROR_NO_ERROR;
}

int custom_SOURCE_PWM_CHANNEL_INVERT_QUERY(bool *invert) {
    *invert = pwm_channel_invert;
    return SCPI_ERROR_NO_ERROR;
}

