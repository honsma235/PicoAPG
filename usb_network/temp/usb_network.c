// Copyright (c) 2022 Cesanta Software Limited
// All rights reserved

#include <lwip/etharp.h>
#include <lwip/ethip6.h>
#include <lwip/init.h>
#include <lwip/ip.h>
#include <lwip/opt.h>
#include <lwip/timeouts.h>
#include <netif/ethernet.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <tusb.h>

#include "usb_network.h"

static struct mg_tcpip_if *s_ifp;
static uint32_t ownip;
static uint32_t netmask;
uint8_t tud_network_mac_address[6] = {2, 0, 1, 2, 3, 0x77}; // MAC will be generated in usb_network_init()


bool tud_network_recv_cb(const uint8_t *buf, uint16_t len) {
  mg_tcpip_qwrite((void *) buf, len, s_ifp);
  // MG_INFO(("RECV %hu", len));
  // mg_hexdump(buf, len);
  tud_network_recv_renew();
  return true;
}

void tud_network_init_cb(void) {}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
  // MG_INFO(("SEND %hu", arg));
  memcpy(dst, ref, arg);
  return arg;
}

static size_t usb_tx(const void *buf, size_t len, struct mg_tcpip_if *ifp) {
  if (!tud_ready()) return 0;
  while (!tud_network_can_xmit(len)) tud_task();
  tud_network_xmit((void *) buf, len);
  (void) ifp;
  return len;
}

static bool usb_poll(struct mg_tcpip_if *ifp, bool s1) {
  (void) ifp;
  tud_task();
  return s1 ? tud_inited() && tud_ready() && tud_connected() : false;
}


void usb_network_init(struct mg_mgr *mgr)
{
  tud_init(PICO_TUD_RHPORT);

  // generate new tud_network_mac_address from pico board_id, as in cyw43_hal_generate_laa_mac()
  /* pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);
  memcpy(tud_network_mac_address, &board_id.id[2], 6);
  tud_network_mac_address[0] &= (uint8_t)~0x1; // unicast
  tud_network_mac_address[0] |= 0x2; // locally administered
  tud_network_mac_address[5] ^= 0x01; // // the lwip virtual MAC address must be different from the host's; toggle the LSB
 */

  // setup USB network
  ownip = MG_IPV4(192, 168, 3, 1);
  netmask = MG_IPV4(255, 255, 255, 0);
  
  struct mg_tcpip_driver driver = {.tx = usb_tx, .poll = usb_poll};
  struct mg_tcpip_if mif = {.mac = {2, 0, 1, 2, 3, 0x77},
                       .ip = mg_htonl(MG_U32(192, 168, 3, 1)),
                       .mask = mg_htonl(MG_U32(255, 255, 255, 0)),
                       .enable_dhcp_server = true,
                       .driver = &driver,
                       .recv_queue.size = 4096};
  //memcpy(mif.mac, tud_network_mac_address, sizeof(tud_network_mac_address));
  s_ifp = &mif;
  mg_tcpip_init(mgr, &mif);
  //tud_init(PICO_TUD_RHPORT);
  //tud_init();
}

void usb_network_deinit()
{
  mg_tcpip_free(s_ifp);
}
