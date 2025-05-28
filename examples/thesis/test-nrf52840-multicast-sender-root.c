#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/routing/routing.h"
#include "dev/button-hal.h" // Include button HAL
#include "dev/leds.h"       // Include leds for potential feedback
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>       // Include for boolean type
#include <stdio.h>         // Include for printf

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#define MAX_PAYLOAD_LEN 120
#define MCAST_SINK_UDP_PORT 3001 /* Host byte order */
#define SEND_INTERVAL (5 * CLOCK_SECOND) /* clock ticks */
#define ITERATIONS 100 /* messages */

/* Start sending messages START_DELAY secs after we start so that routing can converge */
#define START_DELAY 60

static struct uip_udp_conn * mcast_conn;
static char buf[MAX_PAYLOAD_LEN];
static uint32_t seq_id;
static bool is_sending = false; // Flag to control sending

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif

/*---------------------------------------------------------------------------*/
PROCESS(rpl_root_process, "RPL ROOT, Multicast Sender");
AUTOSTART_PROCESSES(&rpl_root_process);
/*---------------------------------------------------------------------------*/
static void
multicast_send(void)
{
  uint32_t id;

  id = uip_htonl(seq_id);
  memset(buf, 0, MAX_PAYLOAD_LEN);
  memcpy(buf, &id, sizeof(seq_id));

  PRINTF("Send to multicast address: ");
  PRINT6ADDR(&mcast_conn->ripaddr);
  PRINTF(", data [0x%08"PRIx32"]", uip_ntohl(*((uint32_t *)buf)));
  PRINTF(", packet_number #%ld \n", seq_id + 1);

  seq_id++;
  uip_udp_packet_send(mcast_conn, buf, sizeof(id));
}
/*---------------------------------------------------------------------------*/
static void
prepare_mcast(void)
{
  uip_ipaddr_t ipaddr;

#if UIP_MCAST6_CONF_ENGINE == UIP_MCAST6_ENGINE_MPL
  uip_ip6addr(&ipaddr, 0xFF03,0,0,0,0,0,0,0xFC);
#else
  uip_ip6addr(&ipaddr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
#endif

  mcast_conn = udp_new(&ipaddr, UIP_HTONS(MCAST_SINK_UDP_PORT), NULL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rpl_root_process, ev, data)
{
  static struct etimer et;
  button_hal_button_t *btn;

  PROCESS_BEGIN();

  PRINTF("Multicast Engine: '%s'\n", UIP_MCAST6.name);
  PRINTF("Press Button 1 to START sending.\n");
  PRINTF("Press Button 2 to STOP sending.\n");
  PRINTF("Device button count: %u.\n", button_hal_button_count); // Print button count [cite: 27]

  NETSTACK_ROUTING.root_start();
  prepare_mcast();

  // Set a long initial timer, will be adjusted when sending starts
  etimer_set(&et, START_DELAY * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    // --- Button Handling ---
    if(ev == button_hal_press_event) {
        btn = (button_hal_button_t *)data;
        // Check for Button 1 (assuming index 0)
        if(btn == button_hal_get_by_index(0)) {
            if(!is_sending) {
                PRINTF("Button 1 pressed - Starting sender.\n");
                is_sending = true;
                seq_id = 0; // Reset sequence ID on start
                etimer_set(&et, SEND_INTERVAL); // Set timer for next send
                leds_single_on(LEDS_LED1); // Indicate sending (optional)
            } else {
                PRINTF("Button 1 pressed - Already sending.\n");
            }
        // Check for Button 2 (assuming index 1)
        } else if(btn == button_hal_get_by_index(1)) {
            if(is_sending) {
                PRINTF("Button 2 pressed - Stopping sender.\n");
                is_sending = false;
                etimer_stop(&et); // Stop the sending timer
                leds_single_off(LEDS_LED1); // Indicate stopped (optional)
            } else {
                PRINTF("Button 2 pressed - Already stopped.\n");
            }
        }
    }
    // --- End Button Handling ---

    // --- Sending Logic ---
    if(is_sending && etimer_expired(&et)) {
      if(seq_id >= ITERATIONS) {
        PRINTF("Reached %d iterations. Stopping sender.\n", ITERATIONS);
        is_sending = false;
        etimer_stop(&et);
        leds_single_off(LEDS_LED1); // Indicate stopped (optional)
      } else {
        multicast_send();
        etimer_set(&et, SEND_INTERVAL);
      }
    }
    // --- End Sending Logic ---
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/