/*
 * Central output state control
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

#endif /* OUTPUT_H */