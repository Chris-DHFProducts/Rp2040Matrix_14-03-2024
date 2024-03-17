// ***************************************************************************
//
#include "stdio.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "i2cPico.h"

i2c_inst_t *i2c_port;
bool i2c_enabled = false;

// ***************************************************************************
//
void I2C_Setup(uint8_t port,uint SDA_pin,uint SCL_pin,uint CLK_Speed)
{
    if(port)
        i2c_port = i2c1;
    else
        i2c_port = i2c0;

    i2c_init(i2c_port,CLK_Speed);

    // Initialize I2C pins
    gpio_set_function(SDA_pin, GPIO_FUNC_I2C);
    gpio_set_function(SCL_pin, GPIO_FUNC_I2C);

    i2c_enabled = true;
}

// ***************************************************************************
//
bool I2C_Read(uint8_t WH_address,uint8_t *Data,uint8_t Number, bool none_stop )
{
    if(!i2c_enabled)
        return(I2C_FAILED);
    
    uint16_t Result = i2c_read_blocking(i2c_port, WH_address, Data, Number, none_stop); 	
    
    if(Result != Number)
    {
        return(I2C_FAILED);
    }
    return(I2C_OK);    
}

// ***************************************************************************
//
bool I2C_Write(uint8_t WH_address,uint8_t *Data, uint8_t Number, bool none_stop)
{
    if(!i2c_enabled)
        return(I2C_FAILED);
    
    int Result = i2c_write_blocking(i2c_port, WH_address, Data,Number , none_stop); 	
    
    if(Result != Number)
    {
        return(I2C_FAILED);
    }
    return(I2C_OK);
}


