/*
 * root-node.c
 * RPL Root - Unicast PRR Test Sender
 */
#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uipopt.h" // For UIP_TTL
#include "sys/clock.h"
#include "dev/button-hal.h" // Added for button support
#include "dev/leds.h"       // Added for visual feedback

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr = {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif /* MAC_CONF_WITH_TSCH */

#include "sys/log.h"
#define LOG_MODULE "RootPRR"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PORT 1903
#define SEND_INTERVAL_UNICAST (1 * CLOCK_SECOND)
#define PACKETS_PER_CHILD 100

static struct simple_udp_connection udp_conn;
static uint32_t root_rx_count = 0; // Counter for packets received by root

// --- Child IP Address Configuration ---
#define NUM_CHILDREN 19
static uip_ipaddr_t child_ips[NUM_CHILDREN];
static const char *child_ip_strings[NUM_CHILDREN] = {
    "fd00::f6ce:363e:1c0f:2d3", "fd00::f6ce:364f:ef87:4e07",
    "fd00::f6ce:364c:9d91:7c24", "fd00::f6ce:3696:2f4f:8666",
    "fd00::f6ce:360a:367b:a9a",  "fd00::f6ce:3621:8bc0:821",
    "fd00::f6ce:366b:c208:ed21", "fd00::f6ce:3604:3aaa:51bc",
    "fd00::f6ce:36ea:7256:11c8", "fd00::f6ce:36ed:84ec:d1a4",
    "fd00::f6ce:36be:a8e:d3a0",  "fd00::f6ce:362d:24a0:fdcd",
    "fd00::f6ce:369a:1a5b:6238", "fd00::f6ce:3630:d744:f2c1",
    "fd00::f6ce:364c:817e:36ad", "fd00::f6ce:36a2:66db:c584",
    "fd00::f6ce:36c2:96e9:6e33", "fd00::f6ce:363f:ef7f:acfd",
    "fd00::f6ce:3618:4b37:a70"
};

static void
init_child_addresses(void)
{
  LOG_INFO("Initializing %d child addresses:\n", NUM_CHILDREN);
  for (int i = 0; i < NUM_CHILDREN; i++) {
    if(uiplib_ipaddrconv(child_ip_strings[i], &child_ips[i]) == 0) {
      LOG_ERR("Failed to parse IP string: %s\n", child_ip_strings[i]);
      // Consider error handling, e.g., marking the IP as invalid
    } else {
      LOG_INFO_(" Child %2d: ", i + 1);
      LOG_INFO_6ADDR(&child_ips[i]);
      LOG_INFO_("\n");
    }
  }
}

// --- Sending Logic State ---
static bool is_sending_active = false;
static uint16_t current_packet_num_overall = 1; // Tracks packet number (1 to PACKETS_PER_CHILD) for current round
static uint8_t current_child_idx_to_send = 0; // Tracks which child to send to in the current round (0 to NUM_CHILDREN-1)

/*---------------------------------------------------------------------------*/
PROCESS(root_prr_process, "RPL Root PRR Sender");
AUTOSTART_PROCESSES(&root_prr_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback_root(struct simple_udp_connection *c,
                     const uip_ipaddr_t *sender_addr,
                     uint16_t sender_port,
                     const uip_ipaddr_t *receiver_addr,
                     uint16_t receiver_port,
                     const uint8_t *data,
                     uint16_t datalen)
{
  root_rx_count++;
  LOG_INFO("ROOT RX %lu: From ", root_rx_count);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_(" Port %u: '%.*s'\n", sender_port, datalen, (char *)data);
  leds_toggle(LEDS_LED3); // Indicate unicast reception at root
}
/*---------------------------------------------------------------------------*/
static void
send_unicast_packet_to_current_child(void)
{
  char payload[60];

  if(!NETSTACK_ROUTING.node_is_reachable() && !NETSTACK_ROUTING.get_root_ipaddr(NULL)) {
      LOG_WARN("Root not part of DAG or no routes, delaying send.\n");
      return; // Skip sending if RPL not ready
  }

  // Construct payload: "Root Pkt <overall_pkt_num_for_child> to Child <child_array_idx+1>"
  // Note: current_packet_num_overall is the Nth packet this child *should* receive.
  sprintf(payload, "Root Pkt #%u for Child %u", current_packet_num_overall, current_child_idx_to_send +1);

  LOG_INFO("ROOT TX: Pkt #%u to Child %d (", current_packet_num_overall, current_child_idx_to_send + 1);
  LOG_INFO_6ADDR(&child_ips[current_child_idx_to_send]);
  LOG_INFO_("): '%s'\n", payload);

  simple_udp_sendto(&udp_conn, payload, strlen(payload), &child_ips[current_child_idx_to_send]);

  // Advance to next child for the current packet number
  current_child_idx_to_send++;
  if (current_child_idx_to_send >= NUM_CHILDREN) {
    current_child_idx_to_send = 0; // Reset child index for next packet number
    current_packet_num_overall++;    // Move to next packet number for all children
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(root_prr_process, ev, data)
{
  static struct etimer periodic_send_timer;
  button_hal_button_t *btn;

  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif

  NETSTACK_ROUTING.root_start();
  init_child_addresses();

  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback_root);

  LOG_INFO("RPL Root for PRR Test initialized.\n");
  LOG_INFO("Default TTL for Hops (UIP_TTL): %u\n", UIP_TTL);
  LOG_INFO("Press Button 1 (idx 0) to START sending %u pkts to %u children.\n", PACKETS_PER_CHILD, NUM_CHILDREN);
  LOG_INFO("Press Button 2 (idx 1) to STOP sending.\n");

  while (1) {
    PROCESS_YIELD(); // Corrected from PROCESS_WAIT_EVENT_UNTIL for broader event handling

    if (ev == button_hal_press_event) {
      btn = (button_hal_button_t *)data;
      if (btn == button_hal_get_by_index(0)) { // Button 1 to START
        if (!is_sending_active) {
          LOG_INFO("Button 1: STARTING Unicast PRR Test.\n");
          is_sending_active = true;
          current_packet_num_overall = 1;
          current_child_idx_to_send = 0;
          etimer_set(&periodic_send_timer, SEND_INTERVAL_UNICAST); // Start sending timer
          leds_on(LEDS_LED1); // LED1 ON indicates sending active
        } else {
          LOG_INFO("Button 1: Sending already active.\n");
        }
      } else if (btn == button_hal_get_by_index(1)) { // Button 2 to STOP
        if (is_sending_active) {
          LOG_INFO("Button 2: STOPPING Unicast PRR Test.\n");
          is_sending_active = false;
          etimer_stop(&periodic_send_timer);
          leds_off(LEDS_LED1); // LED1 OFF
        } else {
          LOG_INFO("Button 2: Sending not active.\n");
        }
      }
    } else if (ev == PROCESS_EVENT_TIMER && data == &periodic_send_timer) {
      if (is_sending_active) {
        if (current_packet_num_overall <= PACKETS_PER_CHILD) {
          send_unicast_packet_to_current_child();
          etimer_reset(&periodic_send_timer); // Reset for next packet in 1 sec
        } else {
          LOG_INFO("PRR Test COMPLETED: %u packets sent to each of %u children.\n", PACKETS_PER_CHILD, NUM_CHILDREN);
          is_sending_active = false;
          leds_off(LEDS_LED1);
          // Optional: Turn on another LED to signify completion, e.g., LEDS_LED2
          leds_on(LEDS_LED2);
        }
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/