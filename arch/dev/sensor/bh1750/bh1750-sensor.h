#ifndef BH1750_SENSOR_H_
#define BH1750_SENSOR_H_

#include "lib/sensors.h"

// BH1750 Command Opcodes (subset, more can be added)
#define BH1750_CMD_CONT_H_RES_MODE     0x10 // Continuously H-Resolution Mode (1 lx res)
#define BH1750_CMD_CONT_H_RES_MODE_2   0x11 // Continuously H-Resolution Mode2 (0.5 lx res)
#define BH1750_CMD_ONE_TIME_H_RES_MODE 0x20 // One Time H-Resolution Mode (1 lx res)

// Default mode and timing for this driver
#define BH1750_SENSOR_DEFAULT_MODE BH1750_CMD_CONT_H_RES_MODE
// Max measurement time for H-Resolution modes is 180ms (datasheet spec)
#define BH1750_SENSOR_MEASUREMENT_TIME_US 180000UL

// Sensor value type
#define BH1750_SENSOR_TYPE_LIGHT   0

// Error return value for .value()
#define BH1750_SENSOR_READING_ERROR -1 // Or use SENSORS_ERROR if preferred

extern const struct sensors_sensor bh1750_sensor;

#endif /* BH1750_SENSOR_H_ */