/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */

#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "usb_network/usb_network.h"
#include "mongoose.h"
#include "scpi_server/scpi_server.h"
#include "common/main_core1.h"

int main() {
    stdio_init_all();

    struct mg_mgr mgr;
    mg_mgr_init(&mgr); // Initialize Mongoose manager
    usb_network_init(&mgr);

    // Launch Core1
    multicore_launch_core1(main_core1_entry);

    // start scpi server
    scpi_server_init(&mgr);

    mg_mdns_listen(&mgr, NULL, "PicoAPG"); // Start mDNS server

    while (true) {
        mg_mgr_poll(&mgr, 0);
    }
}
