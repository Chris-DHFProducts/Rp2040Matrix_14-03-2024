#ifndef __I2C_BUS_SCAN__
#define __I2C_BUS_SCAN__

bool i2c_device_scaned_ok(uint8_t deviceAddress);
void i2c_bus_setup_scan(uint8_t port, uint8_t sda_pin, uint8_t scl_pin, uint32_t CLK_Speed);

#endif // __I2C_BUS_SCAN__