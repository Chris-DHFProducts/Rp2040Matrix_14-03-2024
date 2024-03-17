#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"

#include "CIE1931.h"

//******************************************************************************************
//
volatile float Cie1931_In_Levels;
volatile float Cie1931_Min_Out_Range;
volatile float Cie1931_Max_Out_Range;

//******************************************************************************************
//
uint16_t CIE1931_Init(uint32_t In_Levels, uint32_t Min_Out_Range, uint32_t Max_Out_Range)
{
    Cie1931_In_Levels = (float)In_Levels;
    Cie1931_Min_Out_Range = (float)Min_Out_Range;
    Cie1931_Max_Out_Range = (float)Max_Out_Range;
}

//******************************************************************************************
//
//  Map input brightness levels to a more HUMAN eye responce
//
uint16_t CIE1931(volatile uint16_t Input_Brightness)
{
    volatile double Long_Input = (double)Input_Brightness;
     volatile uint16_t Output_Brigtness;

    Long_Input = Long_Input / (Cie1931_In_Levels - 1.0);

    Long_Input = Long_Input * 100;

    if (Long_Input <= 8)
        Long_Input = Long_Input / 902.3;
    else
        Long_Input = (pow(((Long_Input + 16) / 116.0), 3.0));

    Long_Input = (Long_Input * ((Cie1931_Max_Out_Range- Cie1931_Min_Out_Range) - 1)) + Cie1931_Min_Out_Range;
    Output_Brigtness = ((uint16_t)Long_Input);// & (Cie1931_Max_Out_Range - 1);

    return (Output_Brigtness);
}
/*
#define CIE1931_IN_LEVELS           26589 //4096 // Same as number of ADC steps
#define CIE1931_MIN_OUT_RANGE_GREEN 4
#define CIE1931_MAX_OUT_RANGE_GREEN 2200 //(1u << 15)
//******************************************************************************************
//
uint16_t CIE1931(uint16_t Input_Brightness)
{
    double Long_Input = (double)Input_Brightness;
    uint16_t Output_Brigtness;

    Long_Input = Long_Input / (CIE1931_IN_LEVELS - 1.0);

    Long_Input = Long_Input * 100;

    if (Long_Input <= 8)
        Long_Input = Long_Input / 902.3;
    else
        Long_Input = (pow(((Long_Input + 16) / 116.0), 3.0));

    Long_Input = (Long_Input * ((CIE1931_MAX_OUT_RANGE_GREEN - CIE1931_MIN_OUT_RANGE_GREEN) - 1)) + CIE1931_MIN_OUT_RANGE_GREEN;
    
    Output_Brigtness = ((uint16_t)Long_Input);// & (CIE1931_MAX_OUT_RANGE_GREEN - 1);

    return (Output_Brigtness);
}
*/