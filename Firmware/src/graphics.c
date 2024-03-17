// ***************************************************************************
//
#include "stdio.h"
#include "stdlib.h"

#include "pico/stdlib.h"
#include "graphics.h"
#include "font_7x5.h"

// ***************************************************************************
//
uint8_t *g_Screen_ptr_Addr;
uint16_t g_Max_X;
uint16_t g_Max_Y;
uint32_t g_Ink_Colour;
uint32_t g_Background_Colour;

// ***************************************************************************
//
void g_Init(uint8_t *Screen, uint16_t Max_X, uint16_t Max_Y)
{
    g_Max_X = Max_X;
    g_Max_Y = Max_Y;
    g_Screen_ptr_Addr = Screen;
}
// ***************************************************************************
//
uint8_t *g_Screen_Address()
{
    return (g_Screen_ptr_Addr);
}

// ***************************************************************************
//
void g_Set_Ink_Colour(uint32_t Colour)
{
    g_Ink_Colour = Colour;
}

// ***************************************************************************
//
void g_Set_Background_Colour(uint32_t Colour)
{
    g_Background_Colour = Colour;
}

// ***************************************************************************
//
void g_Colour_Swop()
{
    uint32_t Colour = g_Ink_Colour;

    g_Ink_Colour = g_Background_Colour;
    g_Background_Colour = Colour;
}

// ***************************************************************************
//
void g_Plot(uint16_t X, uint16_t Y, uint32_t Colour)
{
    uint8_t *Screen_ptr;

    if (X < g_Max_X && Y < g_Max_Y)
    {
        Screen_ptr = g_Screen_ptr_Addr;
        Screen_ptr += g_Max_X * 3 * Y;
        Screen_ptr += 3 * X;

        *Screen_ptr++ = (uint8_t)(Colour & 0x000000FF);         // Red
        *Screen_ptr++ = (uint8_t)((Colour & 0x0000FF00) >> 8);  // Green
        *Screen_ptr++ = (uint8_t)((Colour & 0x00FF0000) >> 16); // Blue
    }
}

// ***************************************************************************
//
void g_Clear(uint32_t Colour)
{
    uint8_t lpx;
    uint8_t lpy;

    lpx = g_Max_X;
    while (lpx--)
    {
        lpy = g_Max_Y;
        while (lpy--)
        {
            g_Plot(lpx, lpy, Colour);
        }
    }
}

// ***************************************************************************
//
void g_Char(uint16_t X, uint16_t Y, char Character)
{
    const uint8_t *Char_Ptr;

    uint8_t string_data;
    uint16_t xlp;
    uint16_t ylp;


    // Calculate address of char bitmap
    Char_Ptr = Font_7x5 + (Character - 32) * 5;

    for (xlp = 0; xlp < 5; xlp++)
    {
        string_data = *Char_Ptr;

        for (ylp = 0; ylp < 7; ylp++)
        {
            if (string_data & 0x1)
            {
                g_Plot(xlp + X, ylp + Y, g_Ink_Colour);
            }
            else
            {
                g_Plot(xlp + X, ylp + Y, g_Background_Colour);
            }
            string_data >>= 1;
        }
        Char_Ptr++;
    }
}

// ***************************************************************************
//
void g_String(uint16_t X, uint16_t Y, char *mStr)
{
    while (*mStr)
    {
        g_Char(X, Y, *mStr);
        mStr++;
        X += 6;
    }
}

// ***************************************************************************
//
void g_Uint16_t(uint16_t X, uint16_t Y, uint16_t Value)
{
    char buffer[10];

    sprintf(buffer, "%5d", Value);
    g_String(X, Y, buffer);
}

// ***************************************************************************
//
void g_Float(uint16_t X, uint16_t Y, float Value)
{
    char buffer[20];

    sprintf(buffer, "%5.1f", Value);
    g_String(X, Y, buffer);
}

// ***************************************************************************
//
// Found at 'https://www.thecrazyprogrammer.com/2017/01/bresenhams-line-drawing-algorithm-c-c.html'
//
//
/*
void g_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int32_t dx;
    int32_t dy;
    int32_t p;
    int32_t x;
    int32_t y;

    dx = x1 - x0;
    dy = y1 - y0;

    x = x0;
    y = y0;

    p = 2 * dy - dx;

    while (x < x1)
    {
        if (p >= 0)
        {
            g_Plot(x, y);
            y = y + 1;
            p = p + 2 * dy - 2 * dx;
        }
        else
        {
            g_Plot(x, y);
            p = p + 2 * dy;
        }
        x = x + 1;
    }
}
*/
// https://www.geeksforgeeks.org/mid-point-line-generation-algorithm/
// midPoint function for line generation
void g_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2)
{
    // calculate dx & dy

    int dx = X2 - X1;
    int dy = Y2 - Y1;

    if (dy <= dx)
    {
        // initial value of decision parameter d
        int d = dy - (dx / 2);
        int x = X1, y = Y1;

        // Plot initial given point
        g_Plot(x, y, g_Ink_Colour);

        // iterate through value of X
        while (x < X2)
        {
            x++;

            // E or East is chosen
            if (d < 0)
                d = d + dy;

            // NE or North East is chosen
            else
            {
                d += (dy - dx);
                y++;
            }

            // Plot intermediate points
            g_Plot(x, y, g_Ink_Colour);
        }
    }

    else if (dx < dy)
    {
        // initial value of decision parameter d
        int d = dx - (dy / 2);
        int x = X1, y = Y1;

        // Plot initial given point
        g_Plot(x, y, g_Ink_Colour);

        // iterate through value of X
        while (y < Y2)
        {
            y++;

            // E or East is chosen
            if (d < 0)
                d = d + dx;

            // NE or North East is chosen
            else
            {
                d += (dx - dy);
                x++;
            }

            // Plot intermediate points
            g_Plot(x, y, g_Ink_Colour);
        }
    }
}

// ***************************************************************************
//
void g_Square(uint16_t X, uint16_t Y, uint16_t w, uint16_t h)
{
    uint8_t lpx;
    uint8_t lpy;

    lpx = w;
    while (lpx--)
    {
        lpy = h;
        while (lpy--)
        {
            g_Plot(X + lpx, Y + lpy, g_Ink_Colour);
        }
    }
}


