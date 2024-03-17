#ifndef __AVERAGE__
#define __AVERAGE__

//*******************************************************************************
//
#define AVERAGE_MAX_TABLE 256

//*******************************************************************************
//
void Average_Init(uint16_t New_Value,uint8_t Bit_Size);
void Average_Set(uint16_t New_Value);
uint16_t Average(int16_t New_Value);

//*******************************************************************************
//
#endif //__AVERAGE__