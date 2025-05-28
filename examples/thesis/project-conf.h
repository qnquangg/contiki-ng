#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

// Define test here
#define TEST_NRF52840_MULTICAST_ENABLED
#define TEST_NRF52840_LIGHT_SENSOR_LED_BUTTON_ENABLED

/*
* =======================================================================================
* TEST_NRF52840_LIGHT_SENSOR_LED_BUTTON_ENABLED
* =======================================================================================
*/
#ifdef TEST_NRF52840_LIGHT_SENSOR_LED_BUTTON_ENABLED
/* --- Contiki-NG Standard Log Configuration (for printf) --- */
#define LOG_CONF_LEVEL_APP                         LOG_LEVEL_INFO

/* --- I2C Configuration (using NRFX_TWIM) --- */
#define NRFX_TWIM_ENABLED 1
#define NRFX_TWI_ENABLED 0

// Select TWIM instance (0 or 1 for nRF52840)
#define NRF_TWI_INSTANCE_ID 0 // For BH1750-ARCH

#if (NRF_TWI_INSTANCE_ID == 0)
#define NRFX_TWIM0_ENABLED 1
#define NRFX_TWIM1_ENABLED 0
#elif (NRF_TWI_INSTANCE_ID == 1)
#define NRFX_TWIM0_ENABLED 0
#define NRFX_TWIM1_ENABLED 1
#else
#error "Invalid NRF_TWI_INSTANCE_ID in project-conf.h"
#endif

/*
 * I2C Pin Configuration (SCL, SDA) - BASED ON YOUR SCHEMATIC
 * P1.08 -> SCL => (1 * 32) + 8 = 40
 * P1.09 -> SDA => (1 * 32) + 9 = 41
 */
#ifndef NRF_TWI_SCL_PIN
#define NRF_TWI_SCL_PIN 40 // P1.08
#endif

#ifndef NRF_TWI_SDA_PIN
#define NRF_TWI_SDA_PIN 41 // P1.09
#endif

// I2C Frequency
#ifndef NRF_TWI_FREQUENCY
#define NRF_TWI_FREQUENCY NRF_TWIM_FREQ_100K
#endif

// Button
#define BUTTON_HAL_CONF_WITH_DESCRIPTION 1
#endif

/*
* =======================================================================================
* MQTT_ENABLED
* =======================================================================================
*/
#ifdef MQTT_ENABLED
/*---------------------------------------------------------------------------*/
/* Border Router                          */
/*---------------------------------------------------------------------------*/
/* Set this to 1 for SLIP to output debug information */
#define SLIP_CONF_OUTPUT_DEBUG_INFO       0

/*---------------------------------------------------------------------------*/
/* Network Configuration                          */
/*---------------------------------------------------------------------------*/
/* Use RPL-Lite for routing */
#define NETSTACK_CONF_ROUTING             rpl_lite_driver

/* Enable router functionality and RA sending */
#define UIP_CONF_ROUTER                   1
#define UIP_CONF_ND6_SEND_RA              1
#define UIP_CONF_BUFFER_SIZE              1400

/* Set a long-lived default route */
#define RPL_CONF_DEFAULT_ROUTE_INFINITE_LIFETIME 1

/* Increase log level for border router */
#define RPL_BORDER_ROUTER_CONF_LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
/* MQTT Configuration                            */
/*---------------------------------------------------------------------------*/
/* MQTT Broker IP Address (tunslip6 host) */
#define MQTT_CLIENT_CONF_BROKER_IP_ADDR   "fd00::1"

/* MQTT Broker Port (default: 1883) */
#define MQTT_CLIENT_CONF_BROKER_PORT      1883

/* Set to 0 if not using IBM Watson */
#define MQTT_CLIENT_CONF_WITH_IBM_WATSON  0

/* Set a client ID */
#define MQTT_CLIENT_CONF_CLIENT_ID        "contiki-br-mqtt"

/* Define topics */
#define MQTT_CLIENT_CONF_PUBLISH_TOPIC    "contiki/br/status"
#define MQTT_CLIENT_CONF_SUBSCRIBE_TOPIC  "contiki/br/command"

/* Default publish interval (in seconds) */
#define MQTT_CLIENT_CONF_PUBLISH_INTERVAL (30 * CLOCK_SECOND)

/* Status LED */
#define MQTT_CLIENT_CONF_STATUS_LED       LEDS_GREEN

/* Log level for MQTT */
#define MQTT_CLIENT_CONF_LOG_LEVEL        LOG_LEVEL_INFO
#endif

/*
* =======================================================================================
* TEST_NRF52840_MULTICAST_ENABLED
* =======================================================================================
*/
#ifdef TEST_NRF52840_MULTICAST_ENABLED

#include "net/ipv6/multicast/uip-mcast6-engines.h"

/* Change this to switch engines. Engine codes in uip-mcast6-engines.h */
#ifndef UIP_MCAST6_CONF_ENGINE
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_SMRF
#endif

/* For Imin: Use 16 over CSMA, 64 over Contiki MAC */
#define ROLL_TM_CONF_IMIN_1         64
#define MPL_CONF_DATA_MESSAGE_IMIN  64
#define MPL_CONF_CONTROL_MESSAGE_IMIN  64

#define UIP_MCAST6_ROUTE_CONF_ROUTES 1

/* Code/RAM footprint savings so that things will fit on our device */
#ifndef NETSTACK_MAX_ROUTE_ENTRIES
#define NETSTACK_MAX_ROUTE_ENTRIES   10
#endif

#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 10
#endif

#endif


// =======================================================================================
#endif /* PROJECT_CONF_H_ */