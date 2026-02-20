/*
 * Unified PWM configuration and control
 */

#include <math.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/time.h"

#include "common/trigger.h"
#include "pwm.h"
#include "pwm_gpio.h"
#include "pwm_irq.h"


/* Global PWM configuration instance */
pwm_config_t g_pwm_config;

/* Alarm ID for burst duration */
static alarm_id_t burst_duration_alarm = -1;

static void apply_config_defaults(void) {
    g_pwm_config.reload_runtime_param = true;

    g_pwm_config.state = PWM_STATE_IDLE;

    g_pwm_config.op_mode = PWM_MODE_OFF;
    g_pwm_config.control_mode = PWM_CONTROL_DUTY;

    g_pwm_config.frequency_hz = 10000.0f;
    g_pwm_config.deadtime = 1E-6f;
    g_pwm_config.min_duty = 0.05f;

    for (int i = 0; i < 3; i++) {
        g_pwm_config.phase[i].gpio_ls = -1;
        g_pwm_config.phase[i].gpio_hs = -1;
        g_pwm_config.phase[i].ls_inverted = false;
        g_pwm_config.phase[i].hs_inverted = false;
        g_pwm_config.phase[i].ls_idle = false;
        g_pwm_config.phase[i].hs_idle = false;
        g_pwm_config.phase[i].ls_slice = 0;
        g_pwm_config.phase[i].ls_channel = 0;
        g_pwm_config.phase[i].hs_slice = 0;
        g_pwm_config.phase[i].hs_channel = 1;
    }

    g_pwm_config.phase_duty[0] = 0.5f;
    g_pwm_config.phase_duty[1] = 0.5f;
    g_pwm_config.phase_duty[2] = 0.5f;
    g_pwm_config.mod_index = 0.0f;
    g_pwm_config.phase_angle_deg = 0.0f;
    g_pwm_config.phase_speed_hz = 1.0f;

    g_pwm_config.pwm_enable_mask = 0;
    g_pwm_config.min_duty_with_deadtime = 0.0f;
    g_pwm_config.max_counter = 0;
    g_pwm_config.deadtime_counts_ls = 0;
    g_pwm_config.deadtime_counts_hs = 0;

    burst_duration_alarm = -1;
}

void pwm_init_module(void) {
    pwm_irq_init();
    memset(&g_pwm_config, 0, sizeof(pwm_config_t));
    apply_config_defaults();
    pwm_update_config();
}

static void pwm_irq_setup(int slice) {

    if (slice == g_pwm_config.pwm_irq_slice) {
        return; /* no change */
    }

    /* If already initialized, disable previous IRQ */
    if (g_pwm_config.pwm_irq_slice >= 0) {
        pwm_set_irq_enabled((uint)g_pwm_config.pwm_irq_slice, false);
        g_pwm_config.pwm_irq_slice = -1;
    }

    if (slice < 0) {
        return;
    }

    /* Register IRQ handler for PWM slice */
    g_pwm_config.pwm_irq_slice = slice;
    pwm_set_irq_enabled((uint)slice, true);
}

static inline void pwm_set_channel_polarity(uint slice_num, uint chan, bool inverted) {
    hw_write_masked(&pwm_hw->slice[slice_num].csr,
                    bool_to_bit(inverted) << (chan ? PWM_CH0_CSR_B_INV_LSB : PWM_CH0_CSR_A_INV_LSB),
                    chan ? PWM_CH0_CSR_B_INV_BITS : PWM_CH0_CSR_A_INV_BITS);
}

void pwm_update_config(void) {
    if (g_pwm_config.state == PWM_STATE_RUNNING) {
        return;
    }

    float clock_hz = clock_get_hz(clk_sys); /* system clock */

    /* Calculate required divider for max resolution */
    /* only use interger divider, not fractional */
    /* Use slightly less than 2^16-1 to avoid rounding errors */
    uint16_t clkdiv = ceilf(clock_hz / (g_pwm_config.frequency_hz * 2.0f * 65534.0f));
    if (clkdiv > 255)
        clkdiv = 255; /* max divider is 255 */
    /* Calculate max_counter (= max level = wrap + 1) based on frequency and clkdiv */
    g_pwm_config.max_counter = (uint16_t)roundf(clock_hz / (g_pwm_config.frequency_hz * 2.0f * (float)clkdiv));

    /* Initialize each phase's GPIO pins and PWM slices and calculate enable mask */
    uint32_t mask = 0;
    int pwm_irq_slice = -1; /* reset IRQ slice, will be set to first used slice in loop */
    for (uint8_t phase = 0; phase < g_pwm_config.op_mode; phase++) {
        pwm_phase_config_t *phase_cfg = &g_pwm_config.phase[phase];

        int gpio_ls = phase_cfg->gpio_ls;
        int gpio_hs = phase_cfg->gpio_hs;

        /* Configure low-side GPIO */
        if (gpio_ls >= 0) {
            phase_cfg->ls_slice = pwm_gpio_to_slice_num(gpio_ls);
            phase_cfg->ls_channel = pwm_gpio_to_channel(gpio_ls);

            /* Apply alignment mode phase-correct */
            pwm_set_phase_correct(phase_cfg->ls_slice, true);
            /* Set wrap value based on frequency */
            pwm_set_wrap(phase_cfg->ls_slice, g_pwm_config.max_counter - 1);
            /* Set clock divider, integer only */
            pwm_set_clkdiv_int_frac4(phase_cfg->ls_slice, clkdiv, 0);
            /* Set polarity based on inversion config */
            pwm_set_channel_polarity(phase_cfg->ls_slice, phase_cfg->ls_channel, phase_cfg->ls_inverted);

            /* remember first configured slice for IRQ */
            if (pwm_irq_slice == -1) {
                pwm_irq_slice = phase_cfg->ls_slice;
            }
            /* Calculate enable mask for PWM slices */
            mask |= (1u << phase_cfg->ls_slice);
        }
        /* Configure high-side GPIO */
        if (gpio_hs >= 0) {
            phase_cfg->hs_slice = pwm_gpio_to_slice_num(gpio_hs);
            phase_cfg->hs_channel = pwm_gpio_to_channel(gpio_hs);

            /* Apply alignment mode phase-correct */
            pwm_set_phase_correct(phase_cfg->hs_slice, true);
            /* Set wrap value based on frequency */
            pwm_set_wrap(phase_cfg->hs_slice, g_pwm_config.max_counter - 1);
            /* Set clock divider, integer only */
            pwm_set_clkdiv_int_frac4(phase_cfg->hs_slice, clkdiv, 0);
            /* Set polarity based on inversion config, inverted for high-side */
            pwm_set_channel_polarity(phase_cfg->hs_slice, phase_cfg->hs_channel, !phase_cfg->hs_inverted);

            /* remember first configured slice for IRQ */
            if (pwm_irq_slice == -1) {
                pwm_irq_slice = phase_cfg->hs_slice;
            }
            /* Calculate enable mask for PWM slices */
            mask |= (1u << phase_cfg->hs_slice);
        }
    }

    /* Store PWM enable mask */
    g_pwm_config.pwm_enable_mask = mask;
    pwm_set_idle_state();

    /* Pre-calculate internal values */
    g_pwm_config.min_duty_with_deadtime = g_pwm_config.min_duty + (g_pwm_config.deadtime * g_pwm_config.frequency_hz);
    uint16_t deadtime_counts = (uint16_t)roundf(g_pwm_config.deadtime * g_pwm_config.frequency_hz * (float)g_pwm_config.max_counter * 2.0f);
    g_pwm_config.deadtime_counts_ls = deadtime_counts / 2;
    g_pwm_config.deadtime_counts_hs = (deadtime_counts + 1) / 2;

    /* force reload of runtime parameters */
    g_pwm_config.reload_runtime_param = true;

    /* Re-initialize PWM IRQ handler */
    pwm_irq_setup(pwm_irq_slice);
}

static int64_t burst_duration_alarm_cb(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;
    burst_duration_alarm = -1;
    pwm_abort();
    return 0; /* one-shot */
}

void pwm_trigger_start(void) {
    if (g_pwm_config.op_mode == PWM_MODE_OFF) {
        return;
    }
    if (g_pwm_config.state == PWM_STATE_RUNNING) {
        return;
    }

    if (g_trigger_config.burst_type == BURST_MODE_DURATION) {
        if (burst_duration_alarm != -1 && g_pwm_config.state == PWM_STATE_RUNNING) {
            /* already running with a burst duration, ignore retrigger */
            return;
        } else if (burst_duration_alarm != -1) {
            /* shouldn't happen that we have an active burst duration alarm when not running, but cancel just in case */
            alarm_pool_cancel_alarm(g_trigger_config.alarm_pool, burst_duration_alarm);
        }
        burst_duration_alarm = alarm_pool_add_alarm_in_us(g_trigger_config.alarm_pool, g_trigger_config.burst_duration_sec * 1e6f + 7, burst_duration_alarm_cb, NULL, false);
    }

    g_pwm_config.state = PWM_STATE_RUNNING;

    g_pwm_config.reload_runtime_param = true; /* Force reload on prime */
    pwm_irq_prime();
    pwm_clear_irq(g_pwm_config.pwm_irq_slice);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);
    pwm_set_mask_enabled(g_pwm_config.pwm_enable_mask);
}

/**
 * Transition from RUNNING to IDLE state
 * Disables PWM hardware and applies idle states to all outputs
 */
void __no_inline_not_in_flash_func(pwm_abort)(void) {
    /* Disable all slices immediately */
    irq_set_enabled(PWM_IRQ_WRAP_0, false); // takes quite long to execute :/
    pwm_set_mask_enabled(0);

    pwm_set_idle_state();
    
    /* Reset runtime state */
    g_pwm_config.state = PWM_STATE_IDLE;

    /* Cancel burst duration alarm if active */
    if (burst_duration_alarm != -1) {
        alarm_pool_cancel_alarm(g_trigger_config.alarm_pool, burst_duration_alarm);
        burst_duration_alarm = -1;
    }

    /* retrigger if Immediate mode */
    if (g_trigger_config.source == TRG_SOURCE_IMM) {
        pwm_trigger_start();
    }
}
