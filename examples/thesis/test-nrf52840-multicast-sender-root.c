#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/routing/routing.h"
#include "dev/button-hal.h"
#include "dev/leds.h"
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#define MAX_PAYLOAD_LEN 120
#define MCAST_SINK_UDP_PORT 3001 /* Host byte order, for sending multicast */
#define UNICAST_ROOT_UDP_PORT 3002 /* Host byte order, for receiving unicast */
#define SEND_INTERVAL (3 * CLOCK_SECOND)
#define ITERATIONS 100

#define START_DELAY 60

static struct uip_udp_conn *mcast_conn; // For sending multicast
static struct uip_udp_conn *unicast_server_conn; // For receiving unicast
static char buf[MAX_PAYLOAD_LEN]; // Buffer for sending multicast
static uint32_t seq_id;
static bool is_sending = false;

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif

/*---------------------------------------------------------------------------*/
PROCESS(rpl_root_process, "RPL ROOT, Multicast Sender, Unicast Receiver");
AUTOSTART_PROCESSES(&rpl_root_process);
/*---------------------------------------------------------------------------*/
static void
multicast_send(void)
{
  uint32_t id_bef, id_aft; // For printing before and after ntohl

  id_bef = seq_id; // Use seq_id directly for the data to be sent
  id_aft = uip_htonl(id_bef); // Convert to network byte order for sending

  memset(buf, 0, MAX_PAYLOAD_LEN);
  memcpy(buf, &id_aft, sizeof(id_aft)); // Copy the network byte ordered ID

  PRINTF("Send to multicast address ");
  PRINT6ADDR(&mcast_conn->ripaddr);
  // For printing, convert back to host byte order if reading directly from buf
  // Or print the original seq_id
  PRINTF(", data [0x%08"PRIx32"]", id_bef); // Print the original sequence id
  PRINTF(", packet_number #%"PRIu32" \n", seq_id); // Use PRIu32 for uint32_t

  seq_id++;
  uip_udp_packet_send(mcast_conn, buf, sizeof(id_aft));
}
/*---------------------------------------------------------------------------*/
static bool
prepare_mcast(void)
{
  uip_ipaddr_t ipaddr;

#if UIP_MCAST6_CONF_ENGINE == UIP_MCAST6_ENGINE_MPL
  uip_ip6addr(&ipaddr, 0xFF03,0,0,0,0,0,0,0xFC);
#else
  uip_ip6addr(&ipaddr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
#endif

  mcast_conn = udp_new(&ipaddr, UIP_HTONS(MCAST_SINK_UDP_PORT), NULL);
  if(mcast_conn == NULL) {
    PRINTF("No UDP connection for multicast, exiting!\n");
    // PROCESS_EXIT();
    return false;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static void
tcpip_unicast_handler(void)
{
  char *appdata;
  if(uip_newdata()) {
    appdata = (char *)uip_appdata;
    // Ensure null termination for string printing,
    // uip_datalen() is length of received data.
    // Place null terminator carefully if uip_appdata buffer is shared or its size is tight.
    // A common practice: ensure appdata buffer is large enough or copy to a local buffer.
    // For this example, we'll null-terminate in place if space allows.
    if (uip_datalen() < MAX_PAYLOAD_LEN) { // Or use a more specific buffer size for received data
        appdata[uip_datalen()] = '\0';
    } else {
        appdata[MAX_PAYLOAD_LEN -1] = '\0'; // Fallback if data is too long
    }


    PRINTF("Received unicast from ");
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF(" on port %u: '%s'\n", uip_ntohs(UIP_UDP_BUF->srcport), appdata);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rpl_root_process, ev, data)
{
  static struct etimer periodic_timer;
  button_hal_button_t *btn;

  PROCESS_BEGIN();

  PRINTF("RPL Root Process Started\n");
  PRINTF("Multicast Engine: '%s'\n", UIP_MCAST6.name);
  PRINTF("Press Button 1 to START sending multicast.\n");
  PRINTF("Press Button 2 to STOP sending multicast.\n");
  PRINTF("Device button count: %u.\n", button_hal_button_count);

  NETSTACK_ROUTING.root_start();
  if (!prepare_mcast()) {
    PROCESS_EXIT();
  }

  // Setup unicast server connection
  unicast_server_conn = udp_new(NULL, UIP_HTONS(0), NULL); // Remote port 0 initially for server
  if(unicast_server_conn == NULL) {
    PRINTF("No UDP connection for unicast server, exiting!\n");
    PROCESS_EXIT();
  }
  udp_bind(unicast_server_conn, UIP_HTONS(UNICAST_ROOT_UDP_PORT));
  PRINTF("Unicast server listening on UDP port %u\n", UNICAST_ROOT_UDP_PORT);


  // Set a long initial timer for multicast sending, will be adjusted when sending starts
  etimer_set(&periodic_timer, START_DELAY * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      // Check if the event is for our unicast_server_conn
      // if(uip_udp_conn() == unicast_server_conn) {
      tcpip_unicast_handler();
      // }
      // Could also check for other tcpip events if necessary
    } else if(ev == button_hal_press_event) {
        btn = (button_hal_button_t *)data;
        // Check for Button 1 (assuming index 0 or a specific ID like BUTTON_HAL_ID_BUTTON_ZERO)
        // Using get_by_index as in the original snippet
        if(btn == button_hal_get_by_index(0)) { // Button 1
            if(!is_sending) {
                PRINTF("Button 1 pressed - Starting multicast sender.\n");
                is_sending = true;
                seq_id = 0; // Reset sequence ID on start
                etimer_set(&periodic_timer, SEND_INTERVAL); // Set timer for next send
                leds_single_on(LEDS_LED1);
            } else {
                PRINTF("Button 1 pressed - Multicast sender already running.\n");
            }
        } else if(btn == button_hal_get_by_index(1)) { // Button 2
            if(is_sending) {
                PRINTF("Button 2 pressed - Stopping multicast sender.\n");
                is_sending = false;
                etimer_stop(&periodic_timer); // Stop the sending timer
                leds_single_off(LEDS_LED1);
            } else {
                PRINTF("Button 2 pressed - Multicast sender already stopped.\n");
            }
        }
    } else if(ev == PROCESS_EVENT_TIMER && data == &periodic_timer) { // Check if it's our periodic timer
        if(is_sending) { // Only proceed if sending is active
            if(seq_id >= ITERATIONS) {
                PRINTF("Reached %"PRIu32" multicast iterations. Stopping sender.\n", seq_id);
                is_sending = false;
                etimer_stop(&periodic_timer);
                leds_single_off(LEDS_LED1);
            } else {
                multicast_send();
                etimer_reset(&periodic_timer); // Reset timer for the next interval
            }
        } else {
             // If timer expires but we are not sending, reset it to a long delay or stop it
             // This case might happen if START_DELAY timer expires before button is pressed
             etimer_set(&periodic_timer, CLOCK_SECOND * START_DELAY); // Re-arm with long delay
        }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/