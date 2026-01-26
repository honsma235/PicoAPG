#include <pico/unique_id.h>

#include "tusb.h"
#include "usb_network.h"


// USB Network interface implementation for Mongoose TCP/IP stack
static size_t usb_tx(const void *buf, size_t len, struct mg_tcpip_if *ifp);
static bool usb_poll(struct mg_tcpip_if *ifp, bool s1);

// TCP/IP driver structure
static struct mg_tcpip_driver driver = {.tx = usb_tx, .poll = usb_poll};

// TCP/IP interface structure
static struct mg_tcpip_if mif = {
    .driver = &driver,
    .recv_queue.size = 4096
};

static struct mg_tcpip_if *s_ifp = &mif;

// This is the "Host MAC" and gets assigned to the virtual network adapter on the PC.
// This and the "Device MAC" (Pico's virtual network interface) will be generated in usb_network_init() from Pico's unique ID.
uint8_t tud_network_mac_address[6];  

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

static void generate_mac_address() {
  // generate new tud_network_mac_address from pico board_id, as in cyw43_hal_generate_laa_mac()
  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);
  memcpy(tud_network_mac_address, &board_id.id[2], 6);
  tud_network_mac_address[0] &= (uint8_t)~0x1; // unicast
  tud_network_mac_address[0] |= 0x2; // locally administered

  // Also set the interface MAC used by the TCP/IP stack 
  memcpy(mif.mac, tud_network_mac_address, 6);
  
  // the Host MAC address must be different from the Device's; toggle the LSB1
  tud_network_mac_address[5] ^= 0x01;
}

void usb_network_init(struct mg_mgr *mgr) {
  /* generate and apply MAC to both the USB stack and the mg_tcpip_if */
  generate_mac_address();

  mif.ip = mg_htonl(MG_U32(192, 168, 3, 1));
  mif.mask = mg_htonl(MG_U32(255, 255, 255, 0));
  mif.enable_dhcp_server = true;

  mg_tcpip_init(mgr, &mif);
  tusb_init();
}


void usb_network_deinit() {}