/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * PWM GPIO and Hardware Control
 * 
 * Functions for:
 * - Initializing GPIO pins and PWM slices
 * - Setting idle states on GPIO pins
 * - Applying duty cycles with deadtime factoring
 */

#ifndef PWM_GPIO_H
#define PWM_GPIO_H

#include <stdbool.h>

#include "pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize all PWM slices and GPIO pins based on configuration
 * 
 * This must be called from Core1 on startup to set up the hardware.
 * It configures PWM slices, GPIO functions, and initial states.
 * 
 * */
void pwm_init_hardware(void);

void pwm_set_idle_state(void);

/* Attach PWM GPIOs to PWM function (enable output drive). */
void pwm_outputs_attach(void);

/* Detach PWM GPIOs to Hi-Z (disable output drive). */
void pwm_outputs_detach(void);

/**
 * Check if a given GPIO pin is currently assigned to any PWM phase/channel
 * Returns true if the GPIO is in use, false otherwise.
 */
bool pwm_gpio_in_use(int gpio);

#ifdef __cplusplus
}
#endif

#endif /* PWM_GPIO_H */
