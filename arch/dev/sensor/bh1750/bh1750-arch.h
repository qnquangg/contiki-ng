#ifndef BH1750_ARCH_H_
#define BH1750_ARCH_H_

#include <stdint.h>
#include <stdbool.h>

// Select TWIM instance (0 or 1 for nRF52840)
#define NRF_TWI_INSTANCE_ID 0 // For BH1750-ARCH

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

/**
 * @brief Initialize the I2C peripheral used for the BH1750 sensor.
 *
 * This function should configure the I2C pins and instance.
 * @return true if initialization was successful, false otherwise.
 */
bool bh1750_arch_init(void);

/**
 * @brief Power on the BH1750 sensor via I2C command.
 * @return true if successful, false otherwise.
 */
bool bh1750_arch_power_on(void);

/**
 * @brief Power down the BH1750 sensor via I2C command.
 * @return true if successful, false otherwise.
 */
bool bh1750_arch_power_down(void);

/**
 * @brief Reset the BH1750 sensor via I2C command.
 * @return true if successful, false otherwise.
 */
bool bh1750_arch_reset(void);

/**
 * @brief Set the measurement mode for the BH1750 sensor.
 * @param mode The I2C command byte for the desired mode.
 * @return true if successful, false otherwise.
 */
bool bh1750_arch_set_mode(uint8_t mode);

/**
 * @brief Read the raw light level from the BH1750 sensor.
 * @param raw_level Pointer to store the 16-bit raw light level.
 * @return true if successful, false otherwise.
 */
bool bh1750_arch_read_light_level(uint16_t *raw_level);

#endif /* BH1750_ARCH_H_ */