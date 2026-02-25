/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */

#ifndef APG_INTERNAL_H
#define APG_INTERNAL_H

#include <stdint.h>

#define APG_IDLE_GPIO 29 // On RP2040 there are 30 GPIOs, so the two most significant bits in DBG_PADOUT are hardwired to 0
#define APG_MAX_DATA_POINTS 1024u
#define APG_MAX_BITS 32u

typedef struct {
    uint32_t value; /* PIO-ready 32-bit output word (physical bit positions) */
    uint32_t ticks; /* Duration in SM clock ticks */
} apg_item_t;

extern uint8_t s_phys_for_logical[APG_MAX_BITS];
extern uint8_t s_logical_for_phys[APG_MAX_BITS];
extern uint32_t s_active_mask;  
extern apg_item_t s_data[APG_MAX_DATA_POINTS];


#endif /* APG_INTERNAL_H */