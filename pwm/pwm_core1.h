/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * PWM Core1 Executor
 * 
 * Main PWM generation loop running on Core1.
 * Implements state machine, trigger detection, cycle counting, and
 * real-time PWM waveform generation.
 */

#ifndef PWM_CORE1_H
#define PWM_CORE1_H

#include "pwm_config.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Core1 main entry point for PWM generation
 * 
 * This is the main function to be called via multicore_launch_core1().
 * It initializes PWM hardware, then runs the main event loop that:
 * - Monitors configuration changes from Core0
 * - Detects trigger events
 * - Manages state transitions (IDLE -> RUNNING)
 * - Handles burst/timeout auto-stop conditions
 * - Applies idle states when not running
 * 
 * Directly accesses the global g_pwm_config structure.
 * This function never returns under normal operation.
 */
void pwm_core1_main(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_CORE1_H */
