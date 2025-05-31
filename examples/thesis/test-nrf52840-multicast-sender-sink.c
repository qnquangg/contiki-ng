/*
 * sink-sender-node.c
 * Multicast Packet Sender
 */
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/ipv6/uip-debug.h"
#include "dev/button-hal.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h> // For PRIu32
#include <stdbool.h>

#define DEBUG DEBUG_PRINT

#define MCAST_UDP_PORT 3001       // Destination port for multicast packets
#define SEND_INTERVAL (3 * CLOCK_SECOND)
#define MAX_MCAST_ITERATIONS 100  // Send 100 packets
#define MCAST_PAYLOAD_LEN sizeof(uint32_t)

// UDP Connection for sending multicast
static struct uip_udp_conn *mcast_sender_conn;
static char mcast_payload_buf[MCAST_PAYLOAD_LEN];
static uint32_t mcast_seq_id = 0;
static bool is_mcast_active = false;
static struct etimer mcast_send_timer;

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_IPV6_RPL || !UIP_IPV6_MULTICAST
#error "This example can not work with the current Contiki-NG configuration for RPL and Multicast"
#endif

PROCESS(sink_sender_process, "Sink Mcast TX");
AUTOSTART_PROCESSES(&sink_sender_process);
/*---------------------------------------------------------------------------*/
static bool
prepare_mcast_connection(void)
{
  uip_ipaddr_t mcast_addr;

#if UIP_MCAST6_CONF_ENGINE == UIP_MCAST6_ENGINE_MPL
  uip_ip6addr(&mcast_addr, 0xFF03,0,0,0,0,0,0,0xFC);
#else
  uip_ip6addr(&mcast_addr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
#endif

  mcast_sender_conn = udp_new(&mcast_addr, UIP_HTONS(MCAST_UDP_PORT), NULL);
  if(mcast_sender_conn == NULL) {
    PRINTF("SINK_TX: No UDP conn for MCAST sender, exiting.\n");
    return false;
  }
  PRINTF("SINK_TX: MCAST sender ready. Target group: ");
  PRINT6ADDR(&mcast_addr);
  PRINTF(" Port: %u\n", MCAST_UDP_PORT);
  return true;
}
/*---------------------------------------------------------------------------*/
static void
perform_multicast_send(void)
{
  uint32_t net_seq_id;

  net_seq_id = uip_htonl(mcast_seq_id);
  memcpy(mcast_payload_buf, &net_seq_id, sizeof(net_seq_id));

  PRINTF("SINK_TX: Sending MCAST packet #%" PRIu32 " (0x%08" PRIx32 ")\n",
         mcast_seq_id, mcast_seq_id);

  uip_udp_packet_send(mcast_sender_conn, mcast_payload_buf, MCAST_PAYLOAD_LEN);
  mcast_seq_id++;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sink_sender_process, ev, data)
{
  button_hal_button_t *btn;

  PROCESS_BEGIN();

  PRINTF("Sink Sender Node started.\n");
  PRINTF("Button 1 (idx 0) to START sending. Button 2 (idx 1) to STOP.\n");

  if(!prepare_mcast_connection()) {
    PROCESS_EXIT();
  }

  etimer_stop(&mcast_send_timer);

  while(1) {
    PROCESS_YIELD();

    if(ev == button_hal_press_event) {
      btn = (button_hal_button_t *)data;
      if(btn == button_hal_get_by_index(0)) {
        if(!is_mcast_active) {
          PRINTF("SINK_TX: Button 1 pressed - STARTING multicast sending.\n");
          is_mcast_active = true;
          mcast_seq_id = 0;
          perform_multicast_send();
          etimer_set(&mcast_send_timer, SEND_INTERVAL);
          leds_on(LEDS_LED1);
        } else {
          PRINTF("SINK_TX: Button 1 pressed - Already sending.\n");
        }
      } else if(btn == button_hal_get_by_index(1)) {
        if(is_mcast_active) {
          PRINTF("SINK_TX: Button 2 pressed - STOPPING multicast sending.\n");
          is_mcast_active = false;
          etimer_stop(&mcast_send_timer);
          leds_off(LEDS_LED1);
        } else {
          PRINTF("SINK_TX: Button 2 pressed - Already stopped.\n");
        }
      } else {
         PRINTF("SINK_TX: Button ID %u (index %u) pressed.\n", btn->unique_id, btn->button);
      }
    } else if(ev == PROCESS_EVENT_TIMER && data == &mcast_send_timer) {
      if(is_mcast_active) {
        if(mcast_seq_id < MAX_MCAST_ITERATIONS) {
          perform_multicast_send();
          etimer_reset(&mcast_send_timer);
        } else {
          PRINTF("SINK_TX: Reached %u MCAST iterations. Stopping sender.\n", MAX_MCAST_ITERATIONS);
          is_mcast_active = false;
          etimer_stop(&mcast_send_timer);
          leds_off(LEDS_LED1);
        }
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/