#ifndef __INTERFACES__
#define __INTERFACES__

// I2C
#define I2C_PORT 1
#define I2C_SDA_GPIO 14
#define I2C_SCL_GPIO 15
#define I2C_CLOCK_SPEED 1024 * 100  // 100Khz

//I2C device addresses
#define LIGHT_SENSOR_I2C_ADDRESS 0x29   // HB1730

#endif // __INTERFACES__