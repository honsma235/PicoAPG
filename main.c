/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>

#include "mongoose.h"
#include "usb_network/usb_network.h"
#include "scpi_server/scpi_server.h"
#include "pwm/pwm_config.h"
#include "pwm/pwm_core1.h"


int main()
{
    stdio_init_all();

    struct mg_mgr mgr;
    mg_mgr_init(&mgr); // Initialize Mongoose manager
    usb_network_init(&mgr);
    
    // Initialize PWM configuration with defaults
    pwm_config_init();
    
    // Launch Core1 to execute PWM control loop
    multicore_launch_core1(pwm_core1_main);

    // start scpi server
    scpi_server_init(&mgr);

    mg_mdns_listen(&mgr, NULL, "PicoAPG");  // Start mDNS server

    while (true) {
        mg_mgr_poll(&mgr, 0);
    }
}
