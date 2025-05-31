/*
 * root-node.c
 * RPL Root, Multicast Receiver, Unicast Server
 */
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/routing/routing.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-debug.h"
#include "net/ipv6/uipopt.h" // For UIP_TTL
#include "net/ipv6/uip.h"      // Provides extern struct uip_udp_conn *uip_udp_conn;
#include "dev/button-hal.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define DEBUG DEBUG_PRINT

#define MCAST_UDP_PORT 3001
#define UNICAST_ROOT_UDP_PORT 3002

// Globals for multicast reception stats
static uint16_t total_mcast_received = 0;
static uint8_t last_mcast_ttl = 0;
volatile uint8_t hops;

// UDP Connections
static struct uip_udp_conn *mcast_recv_conn;
static struct uip_udp_conn *unicast_server_conn;

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current Contiki-NG configuration for RPL Root and Multicast"
#endif

PROCESS(rpl_root_node_process, "RPL Root, Mcast RX, Ucast Server");
AUTOSTART_PROCESSES(&rpl_root_node_process);
/*---------------------------------------------------------------------------*/
#if UIP_MCAST6_CONF_ENGINE != UIP_MCAST6_ENGINE_MPL
static uip_ds6_maddr_t *
join_mcast_group(void)
{
  uip_ipaddr_t addr;
  uip_ds6_maddr_t *rv;
  uip_ip6addr(&addr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);

  rv = uip_ds6_maddr_add(&addr);
  if(rv) {
    PRINTF("ROOT: Joined multicast group ");
    PRINT6ADDR(&addr);
    PRINTF("\n");
  } else {
    PRINTF("ROOT: Failed to join multicast group\n");
  }
  return rv;
}
#endif
/*---------------------------------------------------------------------------*/
static void
tcpip_event_handler(void)
{
  char *appdata;

  if(uip_newdata()) {
    // CORRECTED: uip_udp_conn is a global variable, not a function call
    if(uip_udp_conn == mcast_recv_conn) {
      total_mcast_received++;
      last_mcast_ttl = UIP_IP_BUF->ttl;
      leds_toggle(LEDS_LED1);
      PRINTF("ROOT: MCAST RX from ");
      PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
      PRINTF(" TTL=%u, Total MCAST RX=%u, Payload[0]=0x%02x\n",
             last_mcast_ttl, total_mcast_received, ((uint8_t *)uip_appdata)[0]);

    } else if(uip_udp_conn == unicast_server_conn) { // CORRECTED
      appdata = (char *)uip_appdata;
      if (uip_datalen() < 120) {
          appdata[uip_datalen()] = '\0';
      } else {
          appdata[119] = '\0';
      }
      leds_toggle(LEDS_LED2);
      PRINTF("ROOT: UNICAST RX from ");
      PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
      PRINTF(" Port %u: '%s'\n", UIP_HTONS(UIP_UDP_BUF->srcport), appdata);
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rpl_root_node_process, ev, data)
{
  button_hal_button_t *btn;
  

  PROCESS_BEGIN();

  PRINTF("RPL Root Node started.\n");
  PRINTF("Multicast Engine: '%s', Default TTL for Hops: %u\n", UIP_MCAST6.name, UIP_TTL);
  PRINTF("Button 3 (index 2) to print MCAST RX stats.\n");

  NETSTACK_ROUTING.root_start();

  mcast_recv_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  if(mcast_recv_conn == NULL) {
    PRINTF("ROOT: No UDP conn for MCAST RX, exiting.\n");
    PROCESS_EXIT();
  }
  udp_bind(mcast_recv_conn, UIP_HTONS(MCAST_UDP_PORT));
  PRINTF("ROOT: MCAST RX listening on UDP port %u\n", MCAST_UDP_PORT);

#if UIP_MCAST6_CONF_ENGINE != UIP_MCAST6_ENGINE_MPL
  join_mcast_group();
#endif

  unicast_server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  if(unicast_server_conn == NULL) {
    PRINTF("ROOT: No UDP conn for UNICAST server, exiting.\n");
    PROCESS_EXIT();
  }
  udp_bind(unicast_server_conn, UIP_HTONS(UNICAST_ROOT_UDP_PORT));
  PRINTF("ROOT: UNICAST server listening on UDP port %u\n", UNICAST_ROOT_UDP_PORT);

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      tcpip_event_handler();
    } else if(ev == button_hal_press_event) {
      btn = (button_hal_button_t *)data;
      if(btn == button_hal_get_by_index(2)) { // Button 3
        hops = 0;
        if(total_mcast_received > 0 && last_mcast_ttl > 0 && last_mcast_ttl <= UIP_TTL) {
          hops = UIP_TTL - last_mcast_ttl;
        }
        PRINTF("ROOT STATS: Total MCAST RX=%u. Last MCAST TTL=%u, Est. Hops From Source=%u\n",
               total_mcast_received, last_mcast_ttl, hops);
        leds_toggle(LEDS_LED3);
      } else {
        PRINTF("ROOT: Button ID %u (index %u) pressed.\n", btn->unique_id, btn->button);
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/