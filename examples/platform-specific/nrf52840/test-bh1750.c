#include "contiki.h"
#include "sys/etimer.h"
#include <stdio.h>

// Include the generic sensor API header for BH1750
// The build system should find this via MODULES definition
#include "dev/sensor/bh1750/bh1750-sensor.h"

/*---------------------------------------------------------------------------*/
PROCESS(test_bh1750_process, "Test BH1750 Example");
AUTOSTART_PROCESSES(&test_bh1750_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_bh1750_process, ev, data)
{
  static struct etimer periodic_timer;
  static int lux_val;
  static bool sensor_ok = false;

  PROCESS_BEGIN();

  printf("----------------------------------------\r\n");
  printf("APP INFO: BH1750 (New Arch) Example Process Started.\r\n");

  printf("APP INFO: Initializing BH1750 sensor hardware (SENSORS_HW_INIT)...\r\n");
  // The SENSORS_ACTIVATE call will internally try to do SENSORS_HW_INIT
  // if the sensor hasn't been initialized.
  // SENSORS_ACTIVATE(bh1750_sensor);

  // Alternatively, be more explicit:
  if(bh1750_sensor.configure(SENSORS_HW_INIT, 0)) {
    printf("APP INFO: BH1750 HW_INIT successful.\r\n");

    printf("APP INFO: Activating BH1750 sensor...\r\n");
    SENSORS_ACTIVATE(bh1750_sensor); // This calls configure(SENSORS_ACTIVE, 1)

    if(bh1750_sensor.status(SENSORS_READY)) {
      printf("APP INFO: BH1750 sensor activated and ready.\r\n");
      sensor_ok = true;

      // Wait for the first measurement as per sensor datasheet and driver notes
      clock_time_t first_measurement_delay = (BH1750_SENSOR_MEASUREMENT_TIME_US / 1000) * CLOCK_SECOND / 1000;
      if(first_measurement_delay == 0) {
        first_measurement_delay = CLOCK_SECOND / 5; // Default if calculation is zero (e.g. very short time)
      }
      etimer_set(&periodic_timer, first_measurement_delay + (CLOCK_SECOND / 10)); // Add a small margin
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      printf("APP INFO: Initial measurement delay passed.\r\n");

    } else {
      printf("APP ERROR: Failed to activate BH1750 or sensor not ready.\r\n");
    }
  } else {
    printf("APP ERROR: BH1750 HW_INIT failed. Check connections and I2C config.\r\n");
  }


  if(sensor_ok) {
    etimer_set(&periodic_timer, CLOCK_SECOND * 2); // Read every 2 seconds
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);

      lux_val = bh1750_sensor.value(BH1750_SENSOR_TYPE_LIGHT);

      if(lux_val != BH1750_SENSOR_READING_ERROR) {
        printf("APP INFO: Ambient Light: %d lx\r\n", lux_val);
      } else {
        printf("APP ERROR: Error reading BH1750 sensor value.\r\n");
      }
    }
  } else {
     printf("APP ERROR: BH1750 setup failed. Halting example.\r\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/