/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */


#ifndef APG_H
#define APG_H

#include <stdbool.h>
#include <stdint.h>

#include "scpi_server/scpi_enums_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t value;
    double duration_sec;
} apg_value_duration_t;

extern bool g_apg_is_enabled; /* SOUR:APG:STATE */
extern SOURCE_APG_IDLE_MODE_IDLE_MODE_t g_apg_idle_mode;
extern uint32_t g_apg_idle_value;
extern size_t g_apg_data_count;

void apg_init_module(void);

int apg_write_data(apg_value_duration_t *pairs, size_t count, bool append);
int apg_read_data(apg_value_duration_t **pairs, size_t *out_count);

void apg_set_mapping(unsigned int logical_bit, int gpio);
void apg_get_mapping(unsigned int logical_bit, int *gpio);
uint32_t apg_map_logical_to_phys(uint32_t logical_word);

void apg_set_state(bool state);
void apg_update_idle(void);
void apg_trigger_start(void);
void apg_abort(void);
void apg_outputs_update(void);

/**
 * Check if a given GPIO pin is currently assigned to any APG channel.
 * If bit >= 0, usage by that same logical bit is ignored.
 * If bit < 0, any usage is reported.
 */
bool apg_gpio_in_use(int bit, int gpio);

#ifdef __cplusplus
}
#endif

#endif /* APG_H */
