// examples/my-app/child-node.c
#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-maddr.h" // For multicast address management
#include "sys/log.h"
#include "random.h" // For simulating sensor data
#include <stdio.h>
#include <string.h>

#define LOG_MODULE "ChildNode"
#define LOG_LEVEL LOG_LEVEL_INFO

// --- Configuration ---
#define UDP_CLIENT_PORT	8765 // Port this node sends data TO (BR listens on this)
#define UDP_SERVER_PORT	5678 // Port this node listens for commands ON

#define SEND_INTERVAL		  (15 * CLOCK_SECOND) // Send data every 15 seconds
#define START_DELAY         (2 * CLOCK_SECOND)  // Initial delay before sending

// Define the multicast addresses to join and listen on
static char* multicast_group1_addr_str = "ff05::101";
static char* multicast_group2_addr_str = "ff05::102";

static struct simple_udp_connection udp_conn;
static uip_ipaddr_t dest_ipaddr; // Will store the Border Router's IP address

// --- Process Definition ---
PROCESS(child_node_process, "Child Node Process");
AUTOSTART_PROCESSES(&child_node_process);

// --- UDP Receive Callback ---
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  // Check if the packet was sent to one of our subscribed multicast addresses
  bool is_multicast = uip_is_addr_mcast(receiver_addr);
  char sender_str[UIP_IPADDR_LEN_STR];
  char receiver_str[UIP_IPADDR_LEN_STR];
  uip_ipaddr_to_str(sender_addr, sender_str, sizeof(sender_str));
  uip_ipaddr_to_str(receiver_addr, receiver_str, sizeof(receiver_str));


  LOG_INFO("Received %s from %s:%u to %s:%u [%u bytes]: '%.*s'\n",
           is_multicast ? "MULTICAST" : "UNICAST",
           sender_str, sender_port, receiver_str, receiver_port, datalen, datalen, (char *) data);

  if(is_multicast && receiver_port == UDP_SERVER_PORT) {
      // Process the command received via multicast
      // Example: Check which group it was for (optional, could just act)
      uip_ipaddr_t temp_mcast_addr;
      if(uiplib_ipaddrconv(multicast_group1_addr_str, &temp_mcast_addr) &&
         uip_ipaddr_cmp(&temp_mcast_addr, receiver_addr))
      {
          LOG_INFO("Command received for Group 1: %.*s\n", datalen, (char *) data);
          // Add action for group 1 (e.g., change config, toggle LED)
      }
      else if(uiplib_ipaddrconv(multicast_group2_addr_str, &temp_mcast_addr) &&
              uip_ipaddr_cmp(&temp_mcast_addr, receiver_addr))
      {
          LOG_INFO("Command received for Group 2: %.*s\n", datalen, (char *) data);
          // Add action for group 2 (e.g., blink LED)
      } else {
          // Command for some other multicast group we joined?
           LOG_INFO("Command received for UNKNOWN group: %.*s\n", datalen, (char *) data);
      }
      // --- Add Actuator Logic Here ---
      // Example: if strncmp((char *)data, "LED_ON", 6) == 0 { leds_on(LEDS_GREEN); }
  } else {
      LOG_WARN("Received unexpected UDP packet on port %u\n", receiver_port);
  }
}

// --- Process Thread ---
PROCESS_THREAD(child_node_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned counter = 0;
  char buf[64];
  uip_ipaddr_t temp_mcast_addr;

  PROCESS_BEGIN();

  // Initialize UDP connection
  // Listen on SERVER_PORT for commands, send TO CLIENT_PORT on BR
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);
  LOG_INFO("UDP Initialized. Listening on port %d.\n", UDP_SERVER_PORT);

  // --- Join Multicast Groups ---
  // Group 1
  if(uiplib_ipaddrconv(multicast_group1_addr_str, &temp_mcast_addr)) {
    if(uip_ds6_maddr_add(&temp_mcast_addr)) {
        LOG_INFO("Joined Multicast Group 1: %s\n", multicast_group1_addr_str);
    } else {
        LOG_ERR("Failed to join Multicast Group 1\n");
    }
  } else {
      LOG_ERR("Failed to parse Multicast Group 1 address\n");
  }
  // Group 2
  if(uiplib_ipaddrconv(multicast_group2_addr_str, &temp_mcast_addr)) {
    if(uip_ds6_maddr_add(&temp_mcast_addr)) {
        LOG_INFO("Joined Multicast Group 2: %s\n", multicast_group2_addr_str);
    } else {
        LOG_ERR("Failed to join Multicast Group 2\n");
    }
  } else {
      LOG_ERR("Failed to parse Multicast Group 2 address\n");
  }
  // Optional: Join link-local all-nodes multicast (ff02::1) if needed
  // uip_create_linklocal_allnodes_mcast(&temp_mcast_addr);
  // uip_ds6_maddr_add(&temp_mcast_addr);


  // Start timer after a short delay
  etimer_set(&periodic_timer, START_DELAY + (random_rand() % START_DELAY));

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    // --- Send Sensor Data (Unicast to BR) ---
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      // Simulate sensor reading
      int temp_reading = 15 + (random_rand() % 20); // Temp between 15 and 34 C
      int light_reading = random_rand() % 1000;     // Light level 0-999

      // Create JSON-like string (or just simple CSV)
      snprintf(buf, sizeof(buf), "{\"id\": %u, \"temp\": %d, \"light\": %d}",
               counter++, temp_reading, light_reading);

      LOG_INFO("Sending data to ");
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_(" : %s\n", buf);

      // Send UDP packet
      simple_udp_sendto(&udp_conn, buf, strlen(buf), &dest_ipaddr);

    } else {
      LOG_WARN("BR not reachable yet or no route found.\n");
      // Optional: Trigger DAG discovery or wait longer
       NETSTACK_ROUTING.leave_network(); // Try rejoining
       etimer_set(&periodic_timer, CLOCK_SECOND * 5); // Wait longer before retrying join/send
       continue; // Skip resetting the main timer below for now
    }

    // Reset the timer for the next transmission
    etimer_set(&periodic_timer, SEND_INTERVAL);
  } // end while(1)

  PROCESS_END();
}