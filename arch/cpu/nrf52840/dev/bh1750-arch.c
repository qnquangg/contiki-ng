#include "contiki.h" // For project-conf.h defines

// Include the nrf_drv_twi legacy layer API
#include "nrf_drv_twi.h" // This header includes <nrfx.h> which defines ret_code_t (nrfx_err_t)

// Sensor architecture header (this file implements its declarations)
#include "dev/sensor/bh1750/bh1750-arch.h"

#include <stdio.h> // For printf
#include "sys/clock.h" // For clock_delay_us if needed, though direct delays here are minimal

// These configurations are expected from project-conf.h
#ifndef NRF_TWI_INSTANCE_ID
#error "NRF_TWI_INSTANCE_ID not defined in project configuration!"
#endif
#ifndef NRF_TWI_SCL_PIN
#error "NRF_TWI_SCL_PIN not defined!"
#endif
#ifndef NRF_TWI_SDA_PIN
#error "NRF_TWI_SDA_PIN not defined!"
#endif
#ifndef NRF_TWI_FREQUENCY // Should use nrf_drv_twi_frequency_t values
#error "NRF_TWI_FREQUENCY not defined using nrf_drv_twi_frequency_t values (e.g., NRF_DRV_TWI_FREQ_100K)!"
#endif

// Create the TWI driver instance using the macro from nrf_drv_twi.h
// NRF_TWI_INSTANCE_ID comes from project-conf.h (e.g., 0 or 1)
static const nrf_drv_twi_t m_twi_bh1750 = NRF_DRV_TWI_INSTANCE(NRF_TWI_INSTANCE_ID);

static bool i2c_drv_initialized = false;
#define BH1750_ARCH_I2C_ADDRESS 0x23

// Static helper function for I2C write
static bool internal_bh1750_i2c_write(uint8_t command)
{
  ret_code_t err_code;
  // The last parameter 'false' means a STOP condition will be generated.
  err_code = nrf_drv_twi_tx(&m_twi_bh1750, BH1750_ARCH_I2C_ADDRESS, &command, 1, false);

  if (err_code != NRFX_SUCCESS) { // NRFX_SUCCESS is used by nrf_drv layer too
    printf("BH1750-ARCH: nrf_drv_twi_tx failed (cmd 0x%02X), error 0x%X\r\n", command, (unsigned int)err_code);
    return false;
  }
  return true;
}

// Static helper function for I2C read
static bool internal_bh1750_i2c_read(uint8_t *data, uint8_t length)
{
  ret_code_t err_code;
  err_code = nrf_drv_twi_rx(&m_twi_bh1750, BH1750_ARCH_I2C_ADDRESS, data, length);

  if (err_code != NRFX_SUCCESS) {
    printf("BH1750-ARCH: nrf_drv_twi_rx failed, error 0x%X\r\n", (unsigned int)err_code);
    return false;
  }
  return true;
}

bool bh1750_arch_init(void)
{
  if (i2c_drv_initialized) {
    return true;
  }

  printf("BH1750-ARCH: Initializing nrf_drv_twi instance %d...\r\n", NRF_TWI_INSTANCE_ID);

  nrf_drv_twi_config_t twi_bh1750_config = {
     .scl                = NRF_TWI_SCL_PIN,
     .sda                = NRF_TWI_SDA_PIN,
     .frequency          = (nrf_drv_twi_frequency_t)NRF_TWI_FREQUENCY, // Cast from NRF_TWIM_FREQ_*
     .interrupt_priority = 7, // or NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
                                                                // TWI_DEFAULT_CONFIG_IRQ_PRIORITY if defined
     .clear_bus_init     = false, // Standard behavior
     .hold_bus_uninit    = false
  };

  // Event handler is NULL for blocking mode
  ret_code_t err_code = nrf_drv_twi_init(&m_twi_bh1750, &twi_bh1750_config, NULL, NULL);
  if (err_code != NRFX_SUCCESS && err_code != NRFX_ERROR_INVALID_STATE) { // Allow if already initialized
    printf("BH1750-ARCH: nrf_drv_twi_init failed: 0x%X\r\n", (unsigned int)err_code);
    return false;
  }

  nrf_drv_twi_enable(&m_twi_bh1750);
  i2c_drv_initialized = true;
  printf("BH1750-ARCH: nrf_drv_twi instance %d initialized and enabled.\r\n", NRF_TWI_INSTANCE_ID);
  printf("SCL: %lu, SDA: %lu\r\n", (unsigned long)NRF_TWI_SCL_PIN, (unsigned long)NRF_TWI_SDA_PIN);
  return true;
}

bool bh1750_arch_power_on(void)
{
  if (!i2c_drv_initialized) {
    printf("BH1750-ARCH: TWI driver not initialized for power_on.\r\n");
    return false;
  }
  return internal_bh1750_i2c_write(0x01); // BH1750_CMD_POWER_ON from datasheet
}

bool bh1750_arch_power_down(void)
{
  if (!i2c_drv_initialized) {
    // If driver isn't up, can't send command. Consider it implicitly powered down.
    // Or return false to indicate command not sent.
    // For an explicit power down command, the driver should be up.
    printf("BH1750-ARCH: TWI driver not initialized for power_down.\r\n");
    return false;
  }
  return internal_bh1750_i2c_write(0x00); // BH1750_CMD_POWER_DOWN from datasheet
}

bool bh1750_arch_reset(void)
{
  if (!i2c_drv_initialized) {
    printf("BH1750-ARCH: TWI driver not initialized for reset.\r\n");
    return false;
  }
  // Datasheet: Reset command is not acceptable in Power Down mode.
  // The generic sensor layer (bh1750-sensor.c) should handle powering on first if needed.
  return internal_bh1750_i2c_write(0x07); // BH1750_CMD_RESET from datasheet
}

bool bh1750_arch_set_mode(uint8_t mode)
{
  if (!i2c_drv_initialized) {
    printf("BH1750-ARCH: TWI driver not initialized for set_mode.\r\n");
    return false;
  }
  return internal_bh1750_i2c_write(mode);
}

bool bh1750_arch_read_light_level(uint16_t *raw_level)
{
  if (!i2c_drv_initialized) {
    printf("BH1750-ARCH: TWI driver not initialized for read_light_level.\r\n");
    return false;
  }
  uint8_t buffer[2];
  if (internal_bh1750_i2c_read(buffer, 2)) {
    *raw_level = (uint16_t)(buffer[0] << 8) | buffer[1];
    return true;
  }
  return false;
}