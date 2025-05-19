#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "net/ipv6/uiplib.h"
#include "sys/clock.h"

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr = {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif /* MAC_CONF_WITH_TSCH */

#include "sys/log.h"
#define LOG_MODULE "Unicast"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_PORT 1903

#define SEND_INTERVAL (5 * CLOCK_SECOND) // 2 seconds per iteration
#define START_DELAY (60 * CLOCK_SECOND)  // Wait 60s for RPL construction
#define TOTAL_PACKET 150

static struct simple_udp_connection udp_conn;
static uint32_t rx_count1 = 0;
volatile uint8_t child_idx = 0;

/*---------------------------------------------------------------------------*/
// Define children address: fd00::2:1, fd00::2:2,...
#define NUM_CHILDREN 20
static uip_ipaddr_t child_ips[NUM_CHILDREN];

static void
init_child_addresses(void)
{
  int i;
  char ip_str[40];
  for (i = 0; i < NUM_CHILDREN; i++)
  {
    int base_id = i + 2; // Start from 2
    sprintf(ip_str, "fd00::%x:%x:%x:%x", 0x200 + base_id, base_id, base_id, base_id);

    uiplib_ipaddrconv(ip_str, &child_ips[i]);
    LOG_INFO("Child %d IP: %s\n", i + 1, ip_str);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS(root_process, "Sender / RPL ROOT");
AUTOSTART_PROCESSES(&root_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{

  LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count1++;
}
/*---------------------------------------------------------------------------*/
void send_unicast_to_children()
{
  static uint16_t packet_number = 1;
  char payload[50];

  if (packet_number > TOTAL_PACKET)
  {
    // No need to send anymore.
    return;
  }

  sprintf(payload, "Send from Root (ID:1) with packet_number %u", packet_number);

  simple_udp_sendto(&udp_conn, payload, strlen(payload), &child_ips[child_idx]);

  LOG_INFO_("Send to node ID:%d, address is ", child_idx+2);
  uiplib_ipaddr_print(&child_ips[child_idx]);
  LOG_INFO_(" and packet_number #%d\n", packet_number);

  // Reset child index when reaching the end of the list.
  if (child_idx == NUM_CHILDREN - 1)
  {
    child_idx = 0;
    packet_number++;
  }
  else
  {
    child_idx++;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(root_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize Children Address */
  init_child_addresses();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  // etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  etimer_set(&periodic_timer, START_DELAY);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    /* Send to children */
    send_unicast_to_children();

    etimer_set(&periodic_timer, SEND_INTERVAL);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
