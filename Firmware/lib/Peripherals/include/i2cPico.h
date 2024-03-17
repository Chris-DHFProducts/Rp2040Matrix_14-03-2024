#ifndef __I2C__H__
#define __I2C__H__


// ***************************************************************************
// I2C

#define I2C_OK      0
#define I2C_FAILED  1

#define I2C_PORT_0 0
#define I2C_PORT_1 1

void I2C_Setup(uint8_t port,uint SDA_pin,uint SCL_pin,uint CLK_Speed);
bool I2C_Write(uint8_t WH_address,uint8_t *Data,uint8_t Number, bool none_stop);
bool I2C_Read(uint8_t WH_address,uint8_t *Data,uint8_t Number, bool none_stop);

#endif //__I2C__H__