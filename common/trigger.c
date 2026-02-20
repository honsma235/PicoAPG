/*
 * Central trigger manager (initial skeleton)
 */

#include "pico/time.h"

#include "pwm/pwm.h"
#include "trigger.h"

trigger_config_t g_trigger_config;

static repeating_timer_t s_int_timer;
static bool s_int_timer_active = false;
static alarm_id_t s_delay_alarm = -1;

void trigger_init(void) {
    g_trigger_config.source = TRG_SOURCE_BUS;
    g_trigger_config.burst_type = BURST_MODE_CONTINUOUS;
    g_trigger_config.delay_sec = 0.0f;
    g_trigger_config.interval_sec = 0.0f;
    g_trigger_config.burst_ncycles = 1;
    g_trigger_config.burst_duration_sec = 0.01f;
    g_trigger_config.alarm_pool = alarm_pool_create_with_unused_hardware_alarm(16);

    s_int_timer_active = false;
    s_delay_alarm = -1;

    trigger_update_config();
}

/* Call every subsystem that should react to a trigger. This will be hardcoded. */
static void trigger_dispatch_all(void) {
    /* Hardcoded dispatch for now; add APG or others when available. */
    pwm_trigger_start();
}

static int64_t trigger_alarm_cb(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;
    s_delay_alarm = -1;
    trigger_dispatch_all();
    return 0; /* one-shot */
}

static void schedule_with_delay(void) {
    if (s_delay_alarm != -1) {
        /* ignore trigger during delay (already scheduled) */
        return;
    }
    s_delay_alarm = alarm_pool_add_alarm_in_us(g_trigger_config.alarm_pool, g_trigger_config.delay_sec * 1e6f, trigger_alarm_cb, NULL, true);
}

static bool int_timer_cb(repeating_timer_t *rt) {
    (void)rt;
    schedule_with_delay();
    return true; /* continue */
}

void trigger_update_config(void) {
    if (s_int_timer_active) {
        cancel_repeating_timer(&s_int_timer);
        s_int_timer_active = false;
    }
    if (g_trigger_config.source != TRG_SOURCE_INT) {
        return;
    }
    s_int_timer_active = alarm_pool_add_repeating_timer_us(g_trigger_config.alarm_pool, -(g_trigger_config.interval_sec * 1e6f), int_timer_cb, NULL, &s_int_timer);
}

void trigger_abort(void) {
    if (s_delay_alarm != -1) {
        alarm_pool_cancel_alarm(g_trigger_config.alarm_pool, s_delay_alarm);
        s_delay_alarm = -1;
    }
    if (s_int_timer_active) {
        cancel_repeating_timer(&s_int_timer);
        s_int_timer_active = false;
    }
}

void trigger_fire(TRIGGER_SOURCE_TRG_SOURCE_t source) {
    /* Respect configured source. */
    if (g_trigger_config.source != source) {
        return;
    }
    schedule_with_delay();
}