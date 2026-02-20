/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * PWM IRQ Handler Header
 */

#ifndef PWM_IRQ_H
#define PWM_IRQ_H

#include "pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prime PWM compare levels once without relying on an IRQ trigger.
 * Intended to be called from Core1 just before enabling the slice to
 * avoid a long first IRQ entry.
 */
void pwm_irq_prime(void);


void pwm_irq_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_IRQ_H */


