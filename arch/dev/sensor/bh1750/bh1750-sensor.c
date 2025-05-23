#include "contiki.h"
#include "lib/sensors.h"
#include "sys/clock.h" // For clock_delay_us

// Include the architecture-specific header for BH1750
// The path might need adjustment based on final Contiki-NG directory structure rules
// For now, assuming CFLAGS will make arch/cpu/nrf52840/dev available
#include "bh1750-arch.h" // This should point to arch/cpu/nrf52840/dev/bh1750-arch.h
#include "bh1750-sensor.h" // This points to arch/dev/sensor/bh1750/bh1750-sensor.h

#include <stdio.h>

static bool sensor_hw_initialized = false;
static bool sensor_active = false;
static uint8_t current_mode_internal = 0;

/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  uint16_t raw_lux;
  float calculated_lux;

  if(!sensor_active) {
    printf("BH1750-SENSOR: Read from inactive sensor.\r\n");
    return BH1750_SENSOR_READING_ERROR; // Or SENSORS_ERROR
  }

  if(type != BH1750_SENSOR_TYPE_LIGHT) {
    printf("BH1750-SENSOR: Invalid value type %d.\r\n", type);
    return BH1750_SENSOR_READING_ERROR;
  }

  if(!bh1750_arch_read_light_level(&raw_lux)) {
    printf("BH1750-SENSOR: Failed to read from arch layer.\r\n");
    return BH1750_SENSOR_READING_ERROR;
  }

  // Convert raw value to Lux. For H-Resolution modes, divide by 1.2.
  if (current_mode_internal == BH1750_CMD_CONT_H_RES_MODE ||
      current_mode_internal == BH1750_CMD_CONT_H_RES_MODE_2 ||
      current_mode_internal == BH1750_CMD_ONE_TIME_H_RES_MODE) {
    calculated_lux = (float)raw_lux / 1.2f;
  } else { // Assuming L-Resolution modes if others are added
    calculated_lux = (float)raw_lux;
  }
  // printf("BH1750-SENSOR: Raw: %u, Lux: %.2f\r\n", raw_lux, (double)calculated_lux);
  return (int)calculated_lux;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch(type) {
    case SENSORS_HW_INIT:
      printf("BH1750-SENSOR: HW_INIT.\r\n");
      if(bh1750_arch_init()) {
        sensor_hw_initialized = true;
        // Optionally power down sensor after init until activated
        bh1750_arch_power_down();
        return 1;
      }
      printf("BH1750-SENSOR: HW_INIT failed.\r\n");
      sensor_hw_initialized = false;
      return 0;

    case SENSORS_ACTIVE:
      if(!sensor_hw_initialized) {
        printf("BH1750-SENSOR: Cannot activate, HW not initialized.\r\n");
        return 0;
      }
      if(c) { // Activate
        printf("BH1750-SENSOR: Activating...\r\n");
        if(!bh1750_arch_power_on()){
            sensor_active = false; return 0;
        }
        clock_delay_usec(2000); // Delay after power on before setting mode
        if(!bh1750_arch_set_mode(BH1750_SENSOR_DEFAULT_MODE)) {
            bh1750_arch_power_down(); // Try to power down if mode set failed
            sensor_active = false; return 0;
        }
        current_mode_internal = BH1750_SENSOR_DEFAULT_MODE;
        sensor_active = true;
        printf("BH1750-SENSOR: Activated. Mode 0x%02X.\r\n", current_mode_internal);
        // Application should wait ~BH1750_SENSOR_MEASUREMENT_TIME_US after this
      } else { // Deactivate
        printf("BH1750-SENSOR: Deactivating...\r\n");
        bh1750_arch_power_down();
        sensor_active = false;
      }
      return 1;

    default:
      printf("BH1750-SENSOR: Unknown configure type %d.\r\n", type);
      return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch(type) {
    case SENSORS_READY:
      return sensor_hw_initialized && sensor_active;
    case SENSORS_ACTIVE: // Same as SENSORS_READY for this simple driver
      return sensor_hw_initialized && sensor_active;
    default:
      return 0;
  }
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(bh1750_sensor, "bh1750", value, configure, status);