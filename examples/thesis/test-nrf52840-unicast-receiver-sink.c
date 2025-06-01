/*
 * child-node.c
 * Unicast PRR Test Receiver / RPL Member
 */
#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip.h"          // For UIP_IP_BUF
#include "net/ipv6/uipopt.h"       // For UIP_TTL
#include "net/ipv6/uiplib.h"
#include "sys/clock.h"
#include "dev/button-hal.h"        // Added for button support
#include "dev/leds.h"              // Added for visual feedback
#include <stdio.h>                 // For sprintf
#include <string.h>                // For strlen

#include "sys/log.h"
#define LOG_MODULE "ChildPRR"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PORT 1903 // Must match Root's UDP_PORT

static struct simple_udp_connection unicast_conn; // Renamed for clarity
static uint32_t received_from_root_count = 0;
static uint8_t last_calculated_hops = 0;

PROCESS(child_prr_process, "Child PRR Receiver/Reporter");
AUTOSTART_PROCESSES(&child_prr_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback_child(struct simple_udp_connection *c,
                      const uip_ipaddr_t *sender_addr,
                      uint16_t sender_port,
                      const uip_ipaddr_t *receiver_addr,
                      uint16_t receiver_port,
                      const uint8_t *data,
                      uint16_t datalen)
{
  received_from_root_count++;
  uint8_t received_ttl = UIP_IP_BUF->ttl; // Get TTL from the IP header

  if (received_ttl > 0 && received_ttl <= UIP_TTL) {
    last_calculated_hops = UIP_TTL - received_ttl;
  } else {
    last_calculated_hops = 255; // Indicate an unusual TTL or error
    LOG_WARN("CHILD RX: Unusual TTL %u received.\n", received_ttl);
  }
  leds_single_toggle(LEDS_LED3); // Toggle LED3 for each packet received from root

  LOG_INFO("CHILD RX %lu: From ", received_from_root_count);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_(" (TTL:%u, Hops:%u): '%.*s'\n",
           received_ttl, last_calculated_hops, datalen, (char *)data);
}
/*---------------------------------------------------------------------------*/
static void
send_status_to_root(void)
{
  uip_ipaddr_t root_ipaddr;
  char payload[80];

  // Try to get the DAG root's IP address
  if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&root_ipaddr)) {
    sprintf(payload, "Child Status - RX Count: %lu, Last Hops: %u",
            received_from_root_count, last_calculated_hops);

    LOG_INFO("CHILD TX: Sending status to root ");
    LOG_INFO_6ADDR(&root_ipaddr);
    LOG_INFO_(" : '%s'\n", payload);

    simple_udp_sendto(&unicast_conn, payload, strlen(payload), &root_ipaddr);
    leds_single_on(LEDS_LED2); // Turn on LED2 briefly to indicate sending status
    clock_delay_usec(10000); // Brief delay for visual effect
    leds_single_off(LEDS_LED2);

  } else {
    LOG_WARN("CHILD TX: Root not reachable or not in a DAG. Cannot send status.\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(child_prr_process, ev, data)
{
  button_hal_button_t *btn;
  PROCESS_BEGIN();

  simple_udp_register(&unicast_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback_child);

  LOG_INFO("Child PRR Test Node initialized.\n");
  LOG_INFO("Listening on UDP port %u.\n", UDP_PORT);
  LOG_INFO("Default TTL for Hops (UIP_TTL): %u\n", UIP_TTL);
  LOG_INFO("Press Button 1 (idx 0) to send RX status to Root.\n");


  while (1) {
    PROCESS_YIELD(); // Use PROCESS_YIELD() to catch all events

    if (ev == button_hal_press_event) {
      btn = (button_hal_button_t *)data;
      if (btn == button_hal_get_by_index(0)) { // Button 1
        LOG_INFO("Button 1: Preparing to send status to root...\n");
        send_status_to_root();
      }
    }
    // Other event handling can be added here if needed
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/