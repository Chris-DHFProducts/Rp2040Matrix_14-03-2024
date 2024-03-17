#ifndef __GRAPHICS_H_
#define __GRAPHICS_H_

// ***************************************************************************
//
// Basic colours          xxBBGGRR 
#define COLOUR_BLACK    0x00000000
#define COLOUR_BLUE     0x00FF0000
#define COLOUR_RED      0x000000FF
#define COLOUR_GREEN    0x0000FF00
#define COLOUR_CYAN     0x0000FFFF
#define COLOUR_MAGENTA  0x00FF00FF
#define COLOUR_YELLOW   0x00FFFF00
#define COLOUR_WHITE    0x00FFFFFF
#define COLOUR_AMBER    0x0000BFFF
#define COLOUR_GRAY     0x00101010

// ***************************************************************************
//
void g_Init(uint8_t *Screen,uint16_t Max_X, uint16_t Max_Y);
uint8_t *g_Screen_Address();

void g_Set_Ink_Colour(uint32_t Colour);
void g_Set_Background_Colour(uint32_t Colour);
void g_Clear(uint32_t Colour);

void g_Clear();
void g_Plot(uint16_t X, uint16_t Y,uint32_t Colour);
void g_Char(uint16_t X, uint16_t Y,char Character);
void g_String(uint16_t X, uint16_t Y,char *mStr);
void g_Uint16_t(uint16_t X, uint16_t Y,uint16_t Value);
void g_Float(uint16_t X, uint16_t Y,float Value);
void g_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2);
void g_Square(uint16_t X, uint16_t Y, uint16_t w, uint16_t h);
void g_Test(uint8_t x, uint8_t y);


#endif //__GRAPHICS_H_