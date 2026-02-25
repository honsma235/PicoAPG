/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/clocks.h"

#include "apg_internal.h"
#include "apg.h"


static double s_tick_error = 0.0;

uint32_t apg_map_logical_to_phys(uint32_t logical_word) {
    uint32_t mapped = 0;
    uint32_t pending = logical_word;
    while (pending) {
        uint32_t logical_bit = __builtin_stdc_trailing_zeros(pending);
        pending &= pending - 1;
        mapped |= (1u << s_phys_for_logical[logical_bit]);
    }
    return mapped;
}

static uint32_t apg_map_phys_to_logical(uint32_t phys_word) {
    uint32_t logical = 0;
    uint32_t pending = phys_word;
    while (pending) {
        uint32_t phys_bit = __builtin_stdc_trailing_zeros(pending);
        pending &= pending - 1;
        logical |= (1u << s_logical_for_phys[phys_bit]);
    }
    return logical;
}

/**
 * Write APG data points.
 * If append is false, this replaces all existing points. If true, new points are appended to the end of the existing points.
 * Returns 0 on success, -1 if there is not enough capacity to store the new points.
 */
int apg_write_data(apg_value_duration_t *pairs, size_t count, bool append) {
    size_t dst = append ? g_apg_data_count : 0;
    const uint32_t clock_hz = clock_get_hz(clk_sys); /* system clock */
    if (!append) s_tick_error = 0.0; // Reset tick error when not appending

    for (size_t i = 0; i < count; i++) {
        const uint32_t mapped_value = apg_map_logical_to_phys(pairs[i].value);
        double requested_ticks = (pairs[i].duration_sec * (double)clock_hz) + s_tick_error;

        while (requested_ticks > (double)UINT32_MAX) {
            if (dst >= APG_MAX_DATA_POINTS) {
                return -1;
            }
            s_data[dst].value = mapped_value;
            s_data[dst].ticks = UINT32_MAX;
            dst++;
            requested_ticks -= ((double)UINT32_MAX + 3.0); // Account for the 3 tick overhead per point
        }

        if (dst >= APG_MAX_DATA_POINTS) {
            return -1;
        }

        double programmed_ticks_f = round(requested_ticks - 3.0);
        if (programmed_ticks_f < 0.0) {
            programmed_ticks_f = 0.0;
        }

        const uint32_t programmed_ticks = (uint32_t)programmed_ticks_f;
        s_data[dst].value = mapped_value;
        s_data[dst].ticks = programmed_ticks;
        dst++;

        s_tick_error = requested_ticks - ((double)programmed_ticks + 3.0);
    }

    g_apg_data_count = dst;
    return 0;
}



static uint32_t remap_word(uint32_t word, const uint8_t old_logical_for_phys[APG_MAX_BITS]) {
    /* Convert physical word -> logical via old map -> new physical via current map. */
    uint32_t logical = 0;
    uint32_t tmp = word;
    while (tmp) {
        uint32_t phys = __builtin_stdc_trailing_zeros(tmp);
        tmp &= tmp - 1;
        logical |= (1u << old_logical_for_phys[phys]);
    }

    uint32_t mapped = 0;
    uint32_t ltmp = logical;
    while (ltmp) {
        uint32_t logical_bit = __builtin_stdc_trailing_zeros(ltmp);
        ltmp &= ltmp - 1;
        mapped |= (1u << s_phys_for_logical[logical_bit]);
    }

    return mapped;
}

static void apg_remap_data(const uint8_t old_logical_for_phys[APG_MAX_BITS]) {
    for (size_t i = 0; i < g_apg_data_count; i++) {
        uint32_t remapped = remap_word(s_data[i].value, old_logical_for_phys);
        s_data[i].value = remapped;
    }
}

void apg_set_mapping(unsigned int logical_bit, int gpio) {
    /* Back up old mapping to remap stored data if we swap. */
    uint8_t old_logical_for_phys[APG_MAX_BITS];
    memcpy(old_logical_for_phys, s_logical_for_phys, sizeof(old_logical_for_phys));

    const uint8_t current_phys = s_phys_for_logical[logical_bit];

    // If the logical bit was previously mapped to a GPIO, disable that GPIO
    if (gpio == -1) {
        s_active_mask &= ~(1u << current_phys);
        return;
    }

    const uint8_t logical_at_target = s_logical_for_phys[gpio];

    if ((uint8_t)gpio != current_phys) {
        /* Swap to keep mapping bijective. */
        s_phys_for_logical[logical_bit] = (uint8_t)gpio;
        s_phys_for_logical[logical_at_target] = current_phys;
        s_logical_for_phys[gpio] = (uint8_t)logical_bit;
        s_logical_for_phys[current_phys] = logical_at_target;

        apg_remap_data(old_logical_for_phys);
    }

    s_active_mask |= (1u << gpio);
}

void apg_get_mapping(unsigned int logical_bit, int *gpio) {
    const uint8_t phys = s_phys_for_logical[logical_bit];
    if (s_active_mask & (1u << phys)) {
        *gpio = (int)phys;
    } else {
        *gpio = -1;
    }
}

int apg_read_data(apg_value_duration_t **pairs, size_t *out_count) {
    if (pairs == NULL || out_count == NULL) {
        return -1;
    }
    if (*pairs != NULL) {
        return -1;
    }

    *out_count = 0;
    const uint32_t clock_hz = clock_get_hz(clk_sys);
    size_t capacity = 0;

    size_t out_idx = 0;
    for (size_t i = 0; i < g_apg_data_count; i++) {
        const uint32_t logical_value = apg_map_phys_to_logical(s_data[i].value);
        const double duration_sec = ((double)s_data[i].ticks + 3.0) / (double)clock_hz;

        if (out_idx > 0 && (*pairs)[out_idx - 1].value == logical_value) {
            (*pairs)[out_idx - 1].duration_sec += duration_sec;
            continue;
        }

        if (out_idx >= capacity) {
            size_t new_capacity = capacity + 8u;
            apg_value_duration_t *new_pairs = realloc(*pairs, new_capacity * sizeof(apg_value_duration_t));
            if (new_pairs == NULL) {
                return -1;
            }
            *pairs = new_pairs;
            capacity = new_capacity;
        }

        (*pairs)[out_idx].value = logical_value;
        (*pairs)[out_idx].duration_sec = duration_sec;
        out_idx++;
    }

    *out_count = out_idx;
    return 0;
}