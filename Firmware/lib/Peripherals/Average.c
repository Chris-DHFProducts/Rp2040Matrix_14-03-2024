#include <stdio.h>
#include "stdlib.h"
#include "pico/stdlib.h"

#include "Average.h"

//*******************************************************************************
//
uint8_t Average_Bit_Size;
uint16_t *Average_Table;
uint16_t Average_Head;
uint32_t Average_Total;
uint16_t Average_Value;

//*******************************************************************************
//
void Average_Init(uint16_t New_Value,uint8_t Bit_Size)
{
    if(Bit_Size >8)
        Bit_Size = 8;

    Average_Table = calloc(((1<<Bit_Size)-1),sizeof(uint16_t));
    Average_Bit_Size = Bit_Size;
    Average_Set(New_Value);
}

//*******************************************************************************
//
//  Set the stating point for screen brightness
//
void Average_Set(uint16_t New_Value)
{
    uint16_t lp;

    // Make sure the new value is not negtive
    if(New_Value < 0)
        New_Value = 0;

    // Fill average table with current value
    lp = 0x01 << Average_Bit_Size;
    while (lp--)
        Average_Table[lp] = New_Value;

    Average_Head = 0;
    Average_Total = (New_Value << Average_Bit_Size);
    Average_Value = New_Value;

    return;
}

//*******************************************************************************
//
//  Return the average from ADC output
//
uint16_t Average(int16_t New_Value)
{
    // Make sure the new value is not negtive
    if(New_Value < 0)
        New_Value = 0;

    Average_Head++; // Move head onto next value
    Average_Head &= ((0x01 << Average_Bit_Size) - 1);

    // Subtract the old value from total
    Average_Total -= Average_Table[Average_Head];

    // Place the new value into average array
    Average_Table[Average_Head] = (uint16_t) New_Value;

    // Add the new value to total
    Average_Total += (uint16_t) New_Value;

    // Get the average by deviding total by number of readings
    Average_Value = Average_Total >> Average_Bit_Size;

    return (Average_Total >> Average_Bit_Size);
}
