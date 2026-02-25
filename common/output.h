/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdbool.h>

typedef struct {
    bool enabled; /* Global output enable flag (:OUTPut:STATe) */
} output_state_t;

extern output_state_t g_output_state;

/* Initialize output state to safe defaults (disabled). */
void output_init(void);

/* Apply current output state; centralizes the disable path. */
void output_apply_state(void);

/* Global output detach function used to disable a specific GPIO. */
void output_detach(int gpio);

#endif /* OUTPUT_H */