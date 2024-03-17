// *****************************************************************************
//
#include "stdio.h"
#include "pico/stdlib.h"

#include "i2cPico.h"
#include "BH1730.h"

// *****************************************************************************
//
uint8_t BH1730_I2C_Address;

uint32_t BH1730_Reading;

// *****************************************************************************
//
void BH1730_Init(uint8_t i2c_addr)
{
    BH1730_I2C_Address = i2c_addr;
    BH1730_Reading = BH1730_Read();
}

//*******************************************************************************
//
// Read Light level from chip hardware
//
uint32_t BH1730_Read()
{
    uint8_t Data[10];
    uint8_t re_try_count;
    bool finished;

    //-------------------------------------------------------------------------------------
    // Setup control and gain registers
    //  : Interrupt is inactive,
    //  : ADC measurement Type0 and Type1.
    //  : ADC measurement starts.
    //  : ADC power on.
    //  : Gain = 1
    Data[0] = BH1730_CMD | BH1730_REG_CONTROL;
    Data[1] = BH1730_REG_CONTROL_POWER | BH1730_REG_CONTROL_ADC_EN | BH1730_REG_CONTROL_ONE_TIME;
    Data[2] = 0xDA; // reg 1 Timing
    Data[3] = 0x00; // reg 2 ints
    Data[4] = 0x00; // reg 3 TH low LSB
    Data[5] = 0x00; // reg 4        MSB
    Data[6] = 0xFF; // reg 5 TH up LSB
    Data[7] = 0xFF; // reg 6       MSB
    Data[8] = 0x00; // reg 7 Gain

    if (I2C_Write(BH1730_I2C_Address, Data, 9, false))
    {
        return (BH1730_FAILED);
    }

    // Wait for ADC data is valid
    re_try_count = 0;
    finished = false;

    while (!finished)
    {
        Data[0] = BH1730_CMD | BH1730_REG_CONTROL;
        if (I2C_Write(BH1730_I2C_Address, Data, 1, false) == I2C_FAILED)
        {
            return (BH1730_FAILED);
        }
        if (I2C_Read(BH1730_I2C_Address, Data, 1, false) == I2C_FAILED)
        {
            return (BH1730_FAILED);
        }

        if (Data[0] & BH1730_REG_CONTROL_ADC_VALID)
        {
            finished = true;
        }
        else
        {
            sleep_ms(20);
            if (++re_try_count >= BH1730_RET_TIMEOUT)
            {
                return (BH1730_FAILED); // Time out
            }
        }
    }

    //-------------------------------------------------------------------------------------
    // Read data back, 4 bytes using General Calling
    // Visible light  data0 luminance LSB - MSB
    // Infrared light data1 luminance LSB - MSB
    Data[0] = BH1730_CMD | BH1730_REG_DATA0_LOW;
    if (I2C_Write(BH1730_I2C_Address, Data, 1, false) == I2C_FAILED)
    {
        return (BH1730_FAILED);
    }

    if (I2C_Read(BH1730_I2C_Address, Data, 4, false) == I2C_FAILED)
    {
        return (BH1730_FAILED);
    }

    BH1730_Reading = Data[0] + (Data[1] << 8);

    return (BH1730_Reading);
}