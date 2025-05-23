#ifndef BH1750_ARCH_H_
#define BH1750_ARCH_H_

#include <stdint.h>
#include <stdbool.h>

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