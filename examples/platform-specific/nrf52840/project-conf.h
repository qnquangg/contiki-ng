#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

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

/* UART for printf is expected to be set up by platform/board files */

#endif /* PROJECT_CONF_H_ */