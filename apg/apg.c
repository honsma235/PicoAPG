/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 *
 * APG (arbitrary pattern generator) module implementation
 *
 * The APG allows users to define a sequence of <value, duration> pairs to run with precise timing.
 * It does this by using the Pico's PIO and DMA features. Continuos and burst modes are supported,
 * with configurable idle behavior. The module also supports flexible assignment of logical bits (as
 * provided by the user) to physical GPIO pins.
 *
 * The main data structure is the s_data array which holds the value/ticks pairs in a PIO-ready
 * format (physical bit positions, ticks). The public API uses value/duration pairs with logical bit
 * positions and duration in seconds. Data mapping and conversion is mostly handled in apg_data.c.
 *
 * The main PIO program (apg) reads value/ticks pairs and outputs the value on the pins for the
 * specified duration. It is fed the s_data array via the data DMA channel. A control DMA channel is
 * used to restart the data DMA channel continuously for continuous mode. In burst mode, the burst
 * PIO program (apg_burst) is used to generated "control blocks" for the control DMA channel, with
 * n-times the main data and a final idle value.
 *
 */

#include <stdalign.h>

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/sync.h"

#include "apg.h"
#include "apg.pio.h"
#include "apg_internal.h"
#include "common/output.h"
#include "common/trigger.h"

#define PWM_IRQ_DEBUG_ENABLE 1 /* Set to 1 to enable GPIO toggling for IRQ timing measurement (scope) */
#define PWM_IRQ_DEBUG_GPIO 2   /* GPIO toggled at start/end of IRQ for timing measurement (scope) */
#if PWM_IRQ_DEBUG_ENABLE
#define PWM_IRQ_DEBUG_SET(x)               \
    do {                                   \
        gpio_put(PWM_IRQ_DEBUG_GPIO, (x)); \
    } while (0)
#define PWM_IRQ_DEBUG_INIT()                        \
    do {                                            \
        gpio_init(PWM_IRQ_DEBUG_GPIO);              \
        gpio_set_dir(PWM_IRQ_DEBUG_GPIO, GPIO_OUT); \
        gpio_put(PWM_IRQ_DEBUG_GPIO, false);        \
    } while (0)
#else
#define PWM_IRQ_DEBUG_SET(x) \
    do {                     \
    } while (0)
#define PWM_IRQ_DEBUG_INIT() \
    do {                     \
    } while (0)
#endif

#define CS_ENTER() critical_section_enter_blocking(&s_apg_crit_sec)
#define CS_EXIT() critical_section_exit(&s_apg_crit_sec)

/* Public APG module configuration and state */
bool g_apg_is_enabled; /* SOUR:APG:STATE */
SOURCE_APG_IDLE_MODE_IDLE_MODE_t g_apg_idle_mode;
uint32_t g_apg_idle_value;
size_t g_apg_data_count;

/* internal variables */
static critical_section_t s_apg_crit_sec;
apg_item_t s_data[APG_MAX_DATA_POINTS];
uint8_t s_phys_for_logical[APG_MAX_BITS]; /* Logical bit -> physical GPIO/PIO bit (bijective) */
uint8_t s_logical_for_phys[APG_MAX_BITS]; /* Inverse mapping */
uint32_t s_active_mask;                   /* Physical bits enabled for output */
static apg_item_t s_idle_point;
static alarm_id_t s_burst_duration_alarm = -1;
static volatile int32_t s_remaining_cycles;
static PIO s_pio = pio0;
static int s_sm = -1;
static int s_burst_sm = -1;
static int s_prog_offset = -1;
static int s_burst_prog_offset = -1;
static int s_dma_chan = -1;
static int s_dma_ctrl_chan = -1;
dma_channel_config s_dma_ctrl_burst_cfg;
dma_channel_config s_dma_ctrl_cont_cfg;
static bool s_initialized = false;
static uint32_t s_data_ptr = 0;

/* Static assertions to ensure assumptions about DMA data structures are valid. */
static_assert(sizeof(apg_item_t) == 8, "apg_item_t must be 8 bytes");
static_assert(alignof(apg_item_t) == 4, "apg_item_t not 4-byte aligned");

static __force_inline bool apg_is_idle(void) {
    /* To distinguish between IDLE and RUNNING state, we use one bit of the PIO output value
     * which is set only in the idle point (APG_IDLE_GPIO). This way we can check if we are
     * currently idle by reading the PIO output value, without needing to track state in software.
     * The data is limitied to 24 bit and configurable GPIOs are limited to 0..22 on SCPI level,
     * so APG_IDLE_GPIO=29 is always available and doesn't conflict with user mappings.
     */
    return s_pio->dbg_padout & (1u << APG_IDLE_GPIO); /* Check if idle bit is set in PIO output value */
}

void apg_update_idle(void) {
    /* if there is no pattern data, use s_idle_value */
    /* s_idle_value has logical mapping, s_idle_point and s_data have physical*/
    uint32_t word = apg_map_logical_to_phys(g_apg_idle_value);

    if (g_apg_data_count > 0) {
        switch (g_apg_idle_mode) {
        case IDLE_MODE_FIRST:
            word = s_data[0].value;
            break;
        case IDLE_MODE_LAST:
            word = s_data[g_apg_data_count - 1].value;
            break;
        }
    }

    //CS_ENTER();

    s_idle_point.value = word | (1u << APG_IDLE_GPIO); /* Ensure idle bit is set in idle point */

    /* If currently idle, we need to update the PIO with the new idle point. */
    /* Otherwise it wil get picked up at the end of the current cycle. */
    if (apg_is_idle()) {
        pio_sm_put(s_pio, (uint)s_sm, s_idle_point.value);
        pio_sm_put(s_pio, (uint)s_sm, s_idle_point.ticks);
    }

    //CS_EXIT();
}

static void apg_hw_setup(void) {
    /* part of module init, no locking needed */

    /* Lazy-claim SM/PIO program/DMA channel once. */
    if (s_sm < 0) {
        s_sm = pio_claim_unused_sm(s_pio, true);
    }
    if (s_burst_sm < 0) {
        s_burst_sm = pio_claim_unused_sm(s_pio, true);
    }
    if (s_prog_offset < 0) {
        if (!pio_can_add_program(s_pio, &apg_program)) {
            return;
        }
        s_prog_offset = pio_add_program(s_pio, &apg_program);
    }
    if (s_burst_prog_offset < 0) {
        if (!pio_can_add_program(s_pio, &apg_burst_program)) {
            return;
        }
        s_burst_prog_offset = pio_add_program(s_pio, &apg_burst_program);
    }

    if (s_sm >= 0 && s_prog_offset >= 0) {
        pio_sm_config c = apg_program_get_default_config((uint)s_prog_offset);
        pio_sm_init(s_pio, (uint)s_sm, (uint)s_prog_offset, &c);
    }

    if (s_burst_sm >= 0 && s_burst_prog_offset >= 0) {
        pio_sm_config c = apg_burst_program_get_default_config((uint)s_burst_prog_offset);
        pio_sm_init(s_pio, (uint)s_burst_sm, (uint)s_burst_prog_offset, &c);
    }

    if (s_dma_chan < 0) {
        s_dma_chan = dma_claim_unused_channel(true);
    }

    if (s_dma_ctrl_chan < 0) {
        s_dma_ctrl_chan = dma_claim_unused_channel(true);
    }

    if (s_dma_chan >= 0 && s_dma_ctrl_chan >= 0 && s_sm >= 0 && s_burst_sm >= 0) {
        /* data DMA streams packed {value,ticks} words into the TX FIFO. Chains to control DMA when done. */
        dma_channel_config cfg = dma_channel_get_default_config((uint)s_dma_chan);
        channel_config_set_high_priority(&cfg, true);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
        channel_config_set_read_increment(&cfg, true);
        channel_config_set_write_increment(&cfg, false);
        channel_config_set_dreq(&cfg, pio_get_dreq(s_pio, (uint)s_sm, true));
        channel_config_set_chain_to(&cfg, (uint)s_dma_ctrl_chan);
        dma_channel_configure((uint)s_dma_chan, &cfg,
                              &s_pio->txf[s_sm],
                              NULL,
                              0,
                              false);

        /* In burst mode: control DMA reads (trans_count, read_addr) sequence from burst SM and writes to data DMA with trigger. */
        s_dma_ctrl_burst_cfg = dma_channel_get_default_config((uint)s_dma_ctrl_chan);
        channel_config_set_high_priority(&s_dma_ctrl_burst_cfg, true);
        channel_config_set_transfer_data_size(&s_dma_ctrl_burst_cfg, DMA_SIZE_32);
        channel_config_set_read_increment(&s_dma_ctrl_burst_cfg, false);
        channel_config_set_write_increment(&s_dma_ctrl_burst_cfg, true);
        channel_config_set_ring(&s_dma_ctrl_burst_cfg, true, 3);                                      // Wrap around after writing 2^3 (=8) bytes (transfer count + read addr (trigger))
        channel_config_set_dreq(&s_dma_ctrl_burst_cfg, pio_get_dreq(s_pio, (uint)s_burst_sm, false)); // DREQ on burst sm RX FIFO

        /* In continuos mode: control DMA only writes read_addr to data DMA with trigger */
        s_dma_ctrl_cont_cfg = dma_channel_get_default_config((uint)s_dma_ctrl_chan);
        channel_config_set_high_priority(&s_dma_ctrl_cont_cfg, true);
        channel_config_set_transfer_data_size(&s_dma_ctrl_cont_cfg, DMA_SIZE_32);
        channel_config_set_read_increment(&s_dma_ctrl_cont_cfg, false);
        channel_config_set_write_increment(&s_dma_ctrl_cont_cfg, false);
    }
}

void apg_init_module(void) {
    PWM_IRQ_DEBUG_INIT();

    if (!critical_section_is_initialized(&s_apg_crit_sec)) {
        critical_section_init(&s_apg_crit_sec);
    }
    CS_ENTER();

    s_initialized = false;

    g_apg_is_enabled = false;
    g_apg_idle_mode = IDLE_MODE_VALUE;
    g_apg_idle_value = 0;
    g_apg_data_count = 0;
    s_active_mask = 0;
    for (size_t i = 0; i < APG_MAX_BITS; i++) {
        s_phys_for_logical[i] = (uint8_t)i;
        s_logical_for_phys[i] = (uint8_t)i;
    }

    s_idle_point.value = (1u << APG_IDLE_GPIO);
    s_idle_point.ticks = 0;

    s_data_ptr = (uint32_t)&s_data[0];

    apg_hw_setup();
    apg_abort(); // Ensure we are in a clean idle state (PIO SM reset and enabled, DMA stopped, idle point output)

    if (s_dma_chan >= 0 && s_dma_ctrl_chan >= 0 && s_sm >= 0 && s_burst_sm >= 0 && s_prog_offset >= 0 && s_burst_prog_offset >= 0)
        s_initialized = true;

    CS_EXIT();
}

void apg_set_state(bool state) {
    g_apg_is_enabled = state;
    apg_abort(); // Ensure we are in a clean idle state (PIO SM reset and enabled, DMA stopped, idle point output)
    apg_outputs_update();
}

static int64_t __not_in_flash_func(burst_duration_alarm_cb)(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;
    s_burst_duration_alarm = -1;
    apg_abort();
    return 0; /* one-shot */
}

/**
 * Note: may be called before module is fully initialized
 */
__attribute__((flatten))
void __not_in_flash_func(apg_trigger_start)(void) {
    if (!s_initialized || !g_apg_is_enabled || g_apg_data_count == 0) {
        return;
    }

    // no retriggering until the current cycle is done
    if (!apg_is_idle()) {
        return;
    }

    CS_ENTER();

    // Prepare burst duration alarm if needed.
    if (g_trigger_config.burst_type == BURST_MODE_DURATION) {
        if (s_burst_duration_alarm != -1) {
            /* already running with a burst duration, ignore retrigger */
            return;
        } else if (s_burst_duration_alarm != -1) {
            /* shouldn't happen that we have an active burst duration alarm when not running, but cancel just in case */
            alarm_pool_cancel_alarm(g_trigger_config.alarm_pool, s_burst_duration_alarm);
        }
        s_burst_duration_alarm = alarm_pool_add_alarm_in_us(g_trigger_config.alarm_pool, g_trigger_config.burst_duration_sec * 1e6f, burst_duration_alarm_cb, NULL, false);
    }

    pio_sm_set_enabled(s_pio, (uint)s_sm, false);
    // safely abort DMAs (See RP2040-E13 / RP2350-E5)
    hw_clear_bits(&dma_hw->ch[s_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
    hw_clear_bits(&dma_hw->ch[s_dma_ctrl_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
    dma_channel_abort((uint)s_dma_chan);
    dma_channel_abort((uint)s_dma_ctrl_chan);
    hw_set_bits(&dma_hw->ch[s_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
    hw_set_bits(&dma_hw->ch[s_dma_ctrl_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);

    if (g_trigger_config.burst_type == BURST_MODE_NCYCLES) {
        /* NCYCLES mode, prepare PIO burst SM*/
        pio_sm_set_enabled(s_pio, (uint)s_burst_sm, false);
        pio_sm_restart(s_pio, (uint)s_burst_sm);
        pio_sm_clear_fifos(s_pio, (uint)s_burst_sm);
        pio_sm_exec(s_pio, (uint)s_burst_sm, pio_encode_jmp((uint)s_burst_prog_offset));

        // Enable burst SM
        pio_sm_set_enabled(s_pio, (uint)s_burst_sm, true);
        // Load N cycle count into burst SM
        pio_sm_put(s_pio, (uint)s_burst_sm, g_trigger_config.burst_ncycles - 1); // -1, because SM uses "do.. while (x--)" loop
        // Load data transfer count
        pio_sm_put(s_pio, (uint)s_burst_sm, g_apg_data_count * 2u); // value+ticks data pairs
        // Load data read address
        pio_sm_put(s_pio, (uint)s_burst_sm, (uint32_t)&s_data[0]);
        // Load idle point transfer count
        pio_sm_put(s_pio, (uint)s_burst_sm, 2u); // value+tick for idle point
        // Load idle point read address
        pio_sm_put(s_pio, (uint)s_burst_sm, (uint32_t)&s_idle_point);

        /* configure ctrl DMA for burst */
        /* Reads (trans_count, read_addr) sequence from burst SM and writes to data DMA with trigger. */
        dma_channel_configure((uint)s_dma_ctrl_chan, &s_dma_ctrl_burst_cfg,
                              &dma_channel_hw_addr((uint)s_dma_chan)->al3_transfer_count,
                              &s_pio->rxf[s_burst_sm],
                              2,
                              true); // let's go!

    } else {
        /* Set trans_count, as only read_addr gets refreshed by control DMA */
        dma_channel_set_transfer_count((uint)s_dma_chan, g_apg_data_count * 2u, false); // value+duration pairs
        /* Configure control DMA for continuous mode /*
        /* Only writes read_addr to data DMA with trigger */
        dma_channel_configure((uint)s_dma_ctrl_chan, &s_dma_ctrl_cont_cfg,
                              &dma_channel_hw_addr((uint)s_dma_chan)->al3_read_addr_trig,
                              &s_data_ptr,
                              1,
                              true); // let's go!
    }

    // Enable main SM
    pio_sm_set_enabled(s_pio, (uint)s_sm, true);

    CS_EXIT();
}

/**
 * Abort APG generation immediately and switch to idle state.
 * Stops DMA, resets PIO sm and sets idle state output.
 */
__attribute__((flatten))
void __not_in_flash_func(apg_abort)(void) {
    if (!s_initialized) {
        return;
    }

    PWM_IRQ_DEBUG_SET(1);

    CS_ENTER();

    /* Cancel burst duration alarm if active */
    if (s_burst_duration_alarm != -1) {
        alarm_pool_cancel_alarm(g_trigger_config.alarm_pool, s_burst_duration_alarm);
        s_burst_duration_alarm = -1;
    }

    /* stop PIO state machines */
    pio_sm_set_enabled(s_pio, (uint)s_sm, false);
    pio_sm_set_enabled(s_pio, (uint)s_burst_sm, false);

    // safely abort DMAs (See RP2040-E13 / RP2350-E5)
    hw_clear_bits(&dma_hw->ch[s_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
    hw_clear_bits(&dma_hw->ch[s_dma_ctrl_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
    dma_channel_abort((uint)s_dma_chan);
    dma_channel_abort((uint)s_dma_ctrl_chan);
    hw_set_bits(&dma_hw->ch[s_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
    hw_set_bits(&dma_hw->ch[s_dma_ctrl_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);

    /* Reset state machines */
    pio_sm_clear_fifos(s_pio, (uint)s_sm);
    pio_sm_restart(s_pio, (uint)s_sm);
    pio_sm_clear_fifos(s_pio, (uint)s_burst_sm);
    pio_sm_restart(s_pio, (uint)s_burst_sm);
    // Execute jumping to the start of the program to ensure we are at `out pins`
    pio_sm_exec(s_pio, (uint)s_sm, pio_encode_jmp((uint)s_prog_offset));
    pio_sm_set_enabled(s_pio, (uint)s_sm, true);
    // Push the idle value and a duration of 0 (so it output once and then stalls)
    pio_sm_put(s_pio, (uint)s_sm, s_idle_point.value); // Idle point value
    pio_sm_put(s_pio, (uint)s_sm, s_idle_point.ticks); // idle point ticks

    CS_EXIT();

    /* retrigger if Immediate mode */
    if (g_apg_is_enabled && g_trigger_config.source == TRG_SOURCE_IMM) {
        apg_trigger_start();
    }

    PWM_IRQ_DEBUG_SET(0);
}

void apg_outputs_update(void) {
    bool enabled = g_apg_is_enabled && g_output_state.enabled;

    // CS_ENTER();

    /* Set pin direction for configured apg pins */
    if (enabled) {
        pio_sm_set_enabled(s_pio, (uint)s_sm, false);
        pio_sm_set_pindirs_with_mask(s_pio, (uint)s_sm, s_active_mask, s_active_mask); // Set active bits to output
        pio_sm_set_enabled(s_pio, (uint)s_sm, true);
    }

    for (uint gpio = 0; gpio < APG_MAX_BITS; gpio++) {
        if (s_active_mask & (1u << gpio)) {
            if (enabled) {
                pio_gpio_init(s_pio, gpio);
            } else {
                output_detach(gpio);
            }
        }
    }

    // CS_EXIT();
}

bool apg_gpio_in_use(int bit, int gpio) {
    if (gpio < 0) {
        return false;
    }

    // If the GPIO isn't active at all, it's not in use
    if ((s_active_mask & (1u << gpio)) == 0) {
        return false;
    }

    // GPIO is in use
    // If bit < 0, report any usage
    if (bit < 0)
        return true;
    // If bit >= 0, ignore usage by that same logical bit (for remapping)
    return s_logical_for_phys[gpio] != (uint8_t)bit;
}
