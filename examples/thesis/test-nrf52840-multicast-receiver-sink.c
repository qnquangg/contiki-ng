#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"

#include <string.h>
#include <stdio.h> // For sprintf

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

// New includes
#include "dev/button-hal.h"         // For button handling
#include "net/routing/routing.h"    // For RPL routing information
#include "net/ipv6/uip.h"           // For uip_ipaddr_copy
#include "net/ipv6/uipopt.h"        // For UIP_TTL (often 64)
#include "dev/leds.h"

#define MCAST_SINK_UDP_PORT 3001 /* Host byte order */
#define UNICAST_ROOT_UDP_PORT 3002 /* Port for unicast to root, host byte order */

static struct uip_udp_conn *sink_conn;  // For receiving multicast
static struct uip_udp_conn *client_conn; // For sending unicast
static uint16_t count; // Counter for received multicast packets
static uint8_t last_received_ttl = 0; // TTL of the last received multicast packet

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif
/*---------------------------------------------------------------------------*/
PROCESS(mcast_sink_process, "Multicast Sink and Unicast Sender Process");
AUTOSTART_PROCESSES(&mcast_sink_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  if(uip_newdata()) {
    count++;
    last_received_ttl = UIP_IP_BUF->ttl; // Store the TTL of the incoming packet
    PRINTF("Received MCAST: Data [0x%08lx], TTL %u, Total RX %u\n",
           (unsigned long)uip_ntohl((unsigned long) *((uint32_t *)(uip_appdata))),
           last_received_ttl, count);
    leds_single_toggle(LEDS_LED3);
  }
  return;
}
/*---------------------------------------------------------------------------*/
#if UIP_MCAST6_CONF_ENGINE != UIP_MCAST6_ENGINE_MPL
static uip_ds6_maddr_t *
join_mcast_group(void)
{
  uip_ipaddr_t addr;
  uip_ds6_maddr_t *rv;
  const uip_ipaddr_t *default_prefix = uip_ds6_default_prefix();

  /* First, set our v6 global */
  uip_ip6addr_copy(&addr, default_prefix);
  uip_ds6_set_addr_iid(&addr, &uip_lladdr);
  uip_ds6_addr_add(&addr, 0, ADDR_AUTOCONF);

  /*
   * IPHC will use stateless multicast compression for this destination
   * (M=1, DAC=0), with 32 inline bits (1E 89 AB CD)
   */
  uip_ip6addr(&addr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
  rv = uip_ds6_maddr_add(&addr);

  if(rv) {
    PRINTF("Joined multicast group ");
    PRINT6ADDR(&uip_ds6_maddr_lookup(&addr)->ipaddr);
    PRINTF("\n");
  }
  return rv;
}
#endif
/*---------------------------------------------------------------------------*/
static void
send_unicast_to_root(void)
{
  uip_ipaddr_t root_ipaddr;
  char buf[60]; // Increased buffer size for longer message
  uint8_t hops = 0;

  if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&root_ipaddr)) {
    // Calculate estimated hops if multicast packets have been received
    if (count > 0) {
      // Assumes multicast packets originate from a source that uses UIP_TTL
      // Hop calculation: Initial_TTL - Received_TTL.
      // e.g. if UIP_TTL is 64:
      // TTL 64 received -> 0 hops (source is neighbor, used 1 hop slot but is 0 hops away itself)
      // TTL 63 received -> 1 hop
      // TTL 62 received -> 2 hops (as per user example)
      if (last_received_ttl > 0 && last_received_ttl <= UIP_TTL) {
        hops = UIP_TTL - last_received_ttl;
      } else if (last_received_ttl > UIP_TTL) {
        // This case should ideally not happen
        PRINTF("WARN: Last RX TTL (%u) > Default TTL (%u). Hops set to 0.\n",
               last_received_ttl, UIP_TTL);
        hops = 0; // Fallback or indicate error
      }
      // If last_received_ttl is 0 (because no multicast received yet, though count > 0 checks this), hops remains 0.
    }

    sprintf(buf, "MCast Count: %u, Est. Hops: %u", count, hops);

    PRINTF("Sending to root ");
    PRINT6ADDR(&root_ipaddr);
    PRINTF(" : Msg '%s'\n", buf);

    uip_udp_packet_sendto(client_conn, buf, strlen(buf),
                          &root_ipaddr, UIP_HTONS(UNICAST_ROOT_UDP_PORT));
  } else {
    PRINTF("Root not reachable or not in a DAG yet. Cannot send unicast.\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mcast_sink_process, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("Multicast Engine: '%s'\n", UIP_MCAST6.name);
  PRINTF("Process started. I can receive multicast and send unicast on button press.\n");
  PRINTF("UIP_TTL is %u\n", UIP_TTL);


#if UIP_MCAST6_CONF_ENGINE != UIP_MCAST6_ENGINE_MPL
  if(join_mcast_group() == NULL) {
    PRINTF("Failed to join multicast group\n");
    PROCESS_EXIT();
  }
#endif

  count = 0; // Initialize multicast packet counter
  last_received_ttl = 0; // Initialize last received TTL

  sink_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  if(sink_conn == NULL) {
    PRINTF("No UDP connection available for multicast sink, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(sink_conn, UIP_HTONS(MCAST_SINK_UDP_PORT));

  PRINTF("Multicast Listening on local port %u\n", UIP_HTONS(sink_conn->lport));

  client_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  if(client_conn == NULL) {
    PRINTF("No UDP connection available for unicast client, exiting the process!\n");
    PROCESS_EXIT();
  }

  PRINTF("Unicast client ready. Press Button 1 to send to root.\n");

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      // Check if the event is for our sink_conn (multicast)
      // This check is good practice if multiple UDP connections exist on different ports.
      // However, uip_udp_appcalled() or checking uip_conn can be more robust.
      // For this example, assuming tcpip_event for this process is primarily for sink_conn.
      // if(uip_udp_conn() == sink_conn) {
      tcpip_handler();
      // }
    } else if (ev == button_hal_press_event) {
      button_hal_button_t *btn = (button_hal_button_t *)data;
      if(btn != NULL && btn->unique_id == BUTTON_HAL_ID_BUTTON_ZERO) {
        PRINTF("Button 1 pressed (ID: %u). Preparing unicast to root.\n", btn->unique_id);
        send_unicast_to_root();
      } else if (btn != NULL) {
        PRINTF("Another button pressed (ID: %u)\n", btn->unique_id);
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/