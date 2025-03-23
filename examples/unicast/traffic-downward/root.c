#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "net/ipv6/uiplib.h"
#include "sys/clock.h"

#include "sys/log.h"
#define LOG_MODULE "Root/Sender"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_PORT 1903

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;
volatile uint8_t child_idx = 0;

/*---------------------------------------------------------------------------*/
// Define children address: fd00::2:1, fd00::2:2,...
#define NUM_CHILDREN 3
static uip_ipaddr_t child_ips[NUM_CHILDREN];

static void
init_child_addresses(void)
{
  int i;
  char ip_str[40];
  for (i = 0; i < NUM_CHILDREN; i++)
  {
    int base_id = i + 2; // Start from 2
    if (base_id < 10)
    {
      sprintf(ip_str, "fd00::20%u:%u:%u:%u", base_id, base_id, base_id, base_id);
    }
    else
    {
      sprintf(ip_str, "fd00::2%u:%u:%u:%u", base_id, base_id, base_id, base_id);
    }

    uiplib_ipaddrconv(ip_str, &child_ips[i]);
    LOG_INFO("Child %d IP: %s\n", i + 1, ip_str);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS(root_process, "Root");
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
  rx_count++;
}
/*---------------------------------------------------------------------------*/
void send_unicast_to_children()
{
  static clock_time_t send_time;
  static uint16_t msg_count = 0;
  char payload[50];

  send_time = clock_time();
  sprintf(payload, "Msg %u, time %lu", msg_count, (unsigned long)send_time);
  
  simple_udp_sendto(&udp_conn, payload, strlen(payload), &child_ips[child_idx]);

  LOG_INFO_("Sent to child: ");
  uiplib_ipaddr_print(&child_ips[child_idx]);
  LOG_INFO_("\n");

  msg_count++;

  // Reset child index when reaching the end of the list.
  if (child_idx == NUM_CHILDREN - 1)
  {
    child_idx = 0;
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

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize Children Address */
  init_child_addresses();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    /* Send to children */
    send_unicast_to_children();

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
