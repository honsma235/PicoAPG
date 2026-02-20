/*
 * Central trigger manager
 */

#ifndef TRIGGER_H
#define TRIGGER_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/time.h"

#include "scpi_server/scpi_enums_gen.h"


typedef struct {
    TRIGGER_SOURCE_TRG_SOURCE_t source;        /* IMM/INT/BUS (expanded via YAML) */
    SOURCE_BURST_TYPE_BURST_MODE_t burst_type; /* CONTINUOUS/NCYCLES/DURATION */
    float delay_sec;                           /* Delay applied before dispatching a trigger event */
    float interval_sec;                        /* Internal trigger interval */
    uint32_t burst_ncycles;                    /* Number of cycles to generate in NCYCLES mode */
    float burst_duration_sec;                  /* Duration of burst in DURATION mode */
    alarm_pool_t *alarm_pool;               /* Alarm pool for trigger related timers */
} trigger_config_t;

extern trigger_config_t g_trigger_config;

void trigger_init(void);

/* Dispatch a trigger event for the given source; applies delay via pico/time alarm. */
void trigger_fire(TRIGGER_SOURCE_TRG_SOURCE_t source);

/* Update trigger based on current config (INT source). */
void trigger_update_config(void);

/* Cancel any pending delay or internal timers. */
void trigger_abort(void);

#endif /* TRIGGER_H */