// ***************************************************************************
//
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

#include "hub75.pio.h"
#include "hub75.h"

// ***************************************************************************
// Main storage for tile configeration
hub75_t *hub75_ROM;

// ***************************************************************************
//  hub75 PIO State machine used
PIO hub75_RGB_data_pio = pio0;
uint hub75_RGB_data_sm = 0;

PIO hub75_row_pio = pio0;
uint hub75_row_sm = 1;

PIO hub75_dimming_pio = pio0;
uint hub75_dimming_sm = 2;

// ***************************************************************************
// hub75 DMA
int hub75_dma_channel;

uint8_t hub75_Output_Buffer_Page = 0x0;
uint8_t hub75_Current_Scan_Row = 0;

// ***************************************************************************
// Dimming control
uint32_t Dimming_Delay[4]; // 4 Bits = 16*16*16 4096 colours

// bool Hub75_Brightness_Update_flag;
// uint32_t Hub75_Brightness;
// bool hub75_Blank_Image = false; // Setting true will cause a blank image to be displayed
//								// Without changing image data

// Data sent to PIO-sm via DMA buffer A
uint8_t *hub75_Output_Buffer_A_0;
uint8_t *hub75_Output_Buffer_A_1;
uint8_t *hub75_Output_Buffer_A_2;
uint8_t *hub75_Output_Buffer_A_3;

// Data sent to PIO-sm via DMA buffer B
uint8_t *hub75_Output_Buffer_B_0;
uint8_t *hub75_Output_Buffer_B_1;
uint8_t *hub75_Output_Buffer_B_2;
uint8_t *hub75_Output_Buffer_B_3;

uint8_t Current_Output_Buffer = DOUBLE_BUFF_A;
uint8_t Next_Output_Buffer = DOUBLE_BUFF_A;

// ****************************************************************************************
//
void Clear_Output_Buffers(uint8_t Output_Buffer, uint8_t value)
{
	if (Output_Buffer == DOUBLE_BUFF_B) // Currently using buffer B so fill buffer A
	{
		memset(hub75_Output_Buffer_A_0, value, hub75_ROM->Output_Bytes);
		memset(hub75_Output_Buffer_A_1, value, hub75_ROM->Output_Bytes);
		memset(hub75_Output_Buffer_A_2, value, hub75_ROM->Output_Bytes);
		memset(hub75_Output_Buffer_A_3, value, hub75_ROM->Output_Bytes);
	}
	else // if Currently using buffer A so fill buffer B
	{
		memset(hub75_Output_Buffer_B_0, value, hub75_ROM->Output_Bytes);
		memset(hub75_Output_Buffer_B_1, value, hub75_ROM->Output_Bytes);
		memset(hub75_Output_Buffer_B_2, value, hub75_ROM->Output_Bytes);
		memset(hub75_Output_Buffer_B_3, value, hub75_ROM->Output_Bytes);
	}
}

// ****************************************************************************************
//
struct Colour_Order_str Mapping_Colour_Order(uint8_t Colour_Order)
{
	struct Colour_Order_str Colour_Order_result;

	// Default RGB
	Colour_Order_result.Red = 0b001;
	Colour_Order_result.Green = 0b010;
	Colour_Order_result.Blue = 0b100;

	switch (Colour_Order)
	{
	case COLOUR_ORDER_RGB:
		Colour_Order_result.Red = 0b001;
		Colour_Order_result.Green = 0b010;
		Colour_Order_result.Blue = 0b100;
		break;
	case COLOUR_ORDER_RBG:
		Colour_Order_result.Red = 0b001;
		Colour_Order_result.Green = 0b100;
		Colour_Order_result.Blue = 0b010;
		break;
	case COLOUR_ORDER_GRB:
		Colour_Order_result.Red = 0b010;
		Colour_Order_result.Green = 0b001;
		Colour_Order_result.Blue = 0b100;
		break;
	case COLOUR_ORDER_GBR:
		Colour_Order_result.Red = 0b010;
		Colour_Order_result.Green = 0b001;
		Colour_Order_result.Blue = 0b100;
		break;
	case COLOUR_ORDER_BRG:
		Colour_Order_result.Red = 0b100;
		Colour_Order_result.Green = 0b010;
		Colour_Order_result.Blue = 0b001;
		break;
	case COLOUR_ORDER_BGR:
		Colour_Order_result.Red = 0b100;
		Colour_Order_result.Green = 0b001;
		Colour_Order_result.Blue = 0b010;
		break;
	}
	return (Colour_Order_result);
}

// ***************************************************************************
// Remap the colours to a new order for example BRG to RGB
// Note BMP images are mapped as BRG
//
void Colour_ReMap(uint8_t *Screen_Buffer, uint8_t From_Colour_Order, uint8_t To_Colour_Order)
{
	uint32_t lp;
	uint8_t Colour_Red;
	uint8_t Colour_Green;
	uint8_t Colour_Blue;

	// Loop over all pixels
	lp = hub75_ROM->Image_Pixels;
	while (lp--)
	{
		// Get the Current pixels colours
		switch (From_Colour_Order)
		{
		case COLOUR_ORDER_RGB:
			Colour_Red = *Screen_Buffer++;
			Colour_Green = *Screen_Buffer++;
			Colour_Blue = *Screen_Buffer++;
			break;
		case COLOUR_ORDER_RBG:
			Colour_Red = *Screen_Buffer++;
			Colour_Blue = *Screen_Buffer++;
			Colour_Green = *Screen_Buffer++;
			break;
		case COLOUR_ORDER_GRB:
			Colour_Green = *Screen_Buffer++;
			Colour_Red = *Screen_Buffer++;
			Colour_Blue = *Screen_Buffer++;
			break;
		case COLOUR_ORDER_GBR:
			Colour_Green = *Screen_Buffer++;
			Colour_Blue = *Screen_Buffer++;
			Colour_Red = *Screen_Buffer++;
			break;
		case COLOUR_ORDER_BRG:
			Colour_Blue = *Screen_Buffer++;
			Colour_Red = *Screen_Buffer++;
			Colour_Green = *Screen_Buffer++;
			break;
		case COLOUR_ORDER_BGR:
			Colour_Blue = *Screen_Buffer++;
			Colour_Green = *Screen_Buffer++;
			Colour_Red = *Screen_Buffer++;
			break;
		}

		// Reset Buffer pointer address
		Screen_Buffer -= 3;

		// Save colours back in new order
		switch (To_Colour_Order)
		{
		case COLOUR_ORDER_RGB:
			*Screen_Buffer++ = Colour_Red;
			*Screen_Buffer++ = Colour_Green;
			*Screen_Buffer++ = Colour_Blue;
			break;
		case COLOUR_ORDER_RBG:
			*Screen_Buffer++ = Colour_Red;
			*Screen_Buffer++ = Colour_Blue;
			*Screen_Buffer++ = Colour_Green;
			break;
		case COLOUR_ORDER_GRB:
			*Screen_Buffer++ = Colour_Green;
			*Screen_Buffer++ = Colour_Red;
			*Screen_Buffer++ = Colour_Blue;
			break;
		case COLOUR_ORDER_GBR:
			*Screen_Buffer++ = Colour_Green;
			*Screen_Buffer++ = Colour_Blue;
			*Screen_Buffer++ = Colour_Red;
			break;
		case COLOUR_ORDER_BRG:
			*Screen_Buffer++ = Colour_Blue;
			*Screen_Buffer++ = Colour_Red;
			*Screen_Buffer++ = Colour_Green;
			break;
		case COLOUR_ORDER_BGR:
			*Screen_Buffer++ = Colour_Blue;
			*Screen_Buffer++ = Colour_Green;
			*Screen_Buffer++ = Colour_Red;
			break;
		}
	}
}

// ****************************************************************************************
//
uint8_t Mapping_Pixel_Mask(uint8_t Output_Buffer_Number)
{
	uint8_t Pixel_Mask;

	switch (Output_Buffer_Number & 0x3)
	{
	case 0:
		Pixel_Mask = 0b00010000; // 0b00010000;
		break;
	case 1:
		Pixel_Mask = 0b00100000; // 0b00100000;
		break;
	case 2:
		Pixel_Mask = 0b01000000; // 0b01000000;
		break;
	case 3:
		Pixel_Mask = 0b10000000; // 0b10000000
		break;
	}
	return (Pixel_Mask);
}

// *****************************************************************************
//
//
void Hub75_Set_Brightness(uint16_t Light_Level)
{
	uint32_t New_Level;

	New_Level = (Light_Level << 16);
	New_Level += (hub75_ROM->Max_Brightness - Light_Level);
	Dimming_Delay[0x00] = New_Level << 0;

	// Light_Level >>= 0x01;
	New_Level = (Light_Level << 16);
	New_Level += (hub75_ROM->Max_Brightness - Light_Level);
	Dimming_Delay[0x03] = New_Level << 1;

	// Light_Level >>= 0x01;
	New_Level = (Light_Level << 16);
	New_Level += (hub75_ROM->Max_Brightness - Light_Level);
	Dimming_Delay[0x02] = New_Level << 2;

	// Light_Level >>= 0x01;
	New_Level = (Light_Level << 16);
	New_Level += (hub75_ROM->Max_Brightness - Light_Level);
	Dimming_Delay[0x01] = New_Level << 3;
}

// ****************************************************************************************
//
uint8_t *Mapping_Output_Buffer_Address(uint8_t Output_Buffer, uint8_t Output_Buffer_Number)
{
	uint8_t *Output_Buffer_Address;

	// Select the correct frame buffer and corrsponding bit mask
	if (Output_Buffer == 'A') // Then use Buffer 'B'
	{
		switch (Output_Buffer_Number & 0x3)
		{
		case 0:
			Output_Buffer_Address = hub75_Output_Buffer_B_3;
			break;
		case 1:
			Output_Buffer_Address = hub75_Output_Buffer_B_2;
			break;
		case 2:
			Output_Buffer_Address = hub75_Output_Buffer_B_1;
			break;
		case 3:
			Output_Buffer_Address = hub75_Output_Buffer_B_0;
			break;
		}
	}
	else //  Use Buffer 'A'
	{
		switch (Output_Buffer_Number & 0x3)
		{
		case 0:
			Output_Buffer_Address = hub75_Output_Buffer_A_3;
			break;
		case 1:
			Output_Buffer_Address = hub75_Output_Buffer_A_2;
			break;
		case 2:
			Output_Buffer_Address = hub75_Output_Buffer_A_1;
			break;
		case 3:
			Output_Buffer_Address = hub75_Output_Buffer_A_0;
			break;
		}
	}
	return (Output_Buffer_Address);
}

// ****************************************************************************************
//
// Covert the Screen RGB bytes into format needed for output buffers
//
uint8_t Screen_Pixels_To_Output(uint8_t *Screen_Pointer, uint8_t Buffer_Count)
{
	struct Colour_Order_str Colour_Bit_Order;

	uint8_t *Channel_1;
	uint8_t *Channel_2;
	uint8_t Result;
	uint8_t Pixel_Mask;

	Channel_1 = Screen_Pointer;
	Channel_2 = Channel_1 + ((hub75_ROM->Tile_Bytes * hub75_ROM->Tiles_X) >> 1);

	Colour_Bit_Order = Mapping_Colour_Order(hub75_ROM->Colour_Order); // Different tiles can have different order of RBG
	Pixel_Mask = Mapping_Pixel_Mask(Buffer_Count);					  // Each of 4 buffers looks at a different bit in pixel data , used for colour pixel dimming

	// Start with channel 2 , in bottom half of screen
	if (*Channel_2 & Pixel_Mask)
		Result |= Colour_Bit_Order.Red;

	Channel_2++;
	if (*Channel_2 & Pixel_Mask)
		Result |= Colour_Bit_Order.Green;

	Channel_2++;
	if (*Channel_2 & Pixel_Mask)
		Result |= Colour_Bit_Order.Blue;

	// Move channel 2 bits into correct position for output buffer formatting
	Result <<= 3;

	// Add Channel 1
	if (*Channel_1 & Pixel_Mask)
		Result |= Colour_Bit_Order.Red;

	Channel_1++;
	if (*Channel_1 & Pixel_Mask)
		Result |= Colour_Bit_Order.Green;

	Channel_1++;
	if (*Channel_1 & Pixel_Mask)
		Result |= Colour_Bit_Order.Blue;

	return (Result);
}

// ****************************************************************************************
//
// Return position in screen buffer for a given pixel
//
uint8_t *Screen_Buffer_Addreass(uint8_t *Base_Pointer, uint8_t Tile_X, uint8_t Tile_Y, uint Byte_x, uint8_t Pixel_y)
{
	uint16_t Result;

	uint8_t Screen_Pixel_X;
	uint8_t Screen_Pixel_Y;

	// Gat absolute screen positions
	Screen_Pixel_X = (Byte_x << 3) + (Tile_X * hub75_ROM->Tile_Pixel_Width);
	Screen_Pixel_Y = Pixel_y + (Tile_Y * hub75_ROM->Tile_Pixel_Height);

	Result = (Screen_Pixel_X) + (hub75_ROM->Image_Pixel_Width * Screen_Pixel_Y);

	Result *= 3;

	return (Base_Pointer + Result);
}

// ****************************************************************************************
//
uint8_t Output_Pixel_Direction(uint8_t Tile_Byte_X, uint8_t Tile_Pixel_Y)
{
	return (hub75_ROM->Pixel_Direction[Tile_Byte_X + (Tile_Pixel_Y * hub75_ROM->Tile_Pixel_Width / 8)]);
}

// ****************************************************************************************
//
// Return position in output buffer for a given pixel
//
uint8_t *Output_Buffer_Addreass(uint8_t *Base_Pointer, uint8_t Tile_X, uint8_t Tile_Y, uint Byte_x, uint8_t Pixel_y)
{
	volatile uint32_t Result;
	volatile uint8_t Tile_Number;
	volatile uint8_t Offset;
	volatile uint8_t Scan_Line;

	Tile_Number = ((hub75_ROM->Tiles_X * Tile_Y) + Tile_X);

	// Get the offset value from mapping array for current Tile_Pixel_X,Tile_Pixel_Y location
	Scan_Line = (hub75_ROM->Pixel_Mapping[Byte_x + (Pixel_y * (hub75_ROM->Tile_Pixel_Width / 8))]);

	// Split values from upper and lower nibbles.
	Offset = Scan_Line >> 4;
	Scan_Line &= 0x0F;

	Result = Scan_Line * (hub75_ROM->Output_Scan_Bytes / 8) + Offset + (Tile_Number * ((hub75_ROM->Output_Scan_Bytes / 8) / hub75_ROM->Tiles));

	Result <<= 3;

	return (Base_Pointer + Result);
}

// ****************************************************************************************
//
// Copy new image to buffers , Double buffering is used, this new image will be displayed
// after the last image has been sent to tiles with 'void hub75_dma_handler()'
//
void Hub75_Update(uint8_t *Screen_Buffer)
{
	volatile uint8_t Colour_Bit_Buffer; //
	volatile uint8_t Tile_X;			// Number of Tiles across screen loop counter
	volatile uint8_t Tile_Y;			// Number of Tiles down screen loop counter
	volatile uint8_t Tile_Byte_X;		// Single tile byte scross loop counter , Loop does 8 pixels at a time
	volatile uint8_t Tile_Pixel_Y;		// Single tile pixel down loop counter
	volatile uint8_t Pixel;				//

	uint8_t *Output_Base;
	uint8_t *Output_Pointer;
	uint8_t *Screen_Pointer;
	uint8_t Pixel_Direction;

	// If the Screen_Buffer is NULL then blank the buffer
	if (Screen_Buffer == NULL)
		Clear_Output_Buffers(Current_Output_Buffer, 0x00);
	else // Update buffers with image data
	{
		for (Colour_Bit_Buffer = 0; Colour_Bit_Buffer < 4; Colour_Bit_Buffer++)
		{
			// Get the Correct output buffer base address
			Output_Base = Mapping_Output_Buffer_Address(Current_Output_Buffer, Colour_Bit_Buffer);

			// Loop over all pixels in image   Panel_Y - Panel_X - X(Bytes) - Y - Pixels
			for (Tile_Y = 0; Tile_Y < hub75_ROM->Tiles_Y; Tile_Y++)
			{
				for (Tile_X = 0; Tile_X < hub75_ROM->Tiles_X; Tile_X++)
				{
					for (Tile_Pixel_Y = 0; Tile_Pixel_Y < hub75_ROM->Tile_Pixel_Height >> 1; Tile_Pixel_Y++)
					{
						for (Tile_Byte_X = 0; Tile_Byte_X < hub75_ROM->Tile_Pixel_Width >> 3; Tile_Byte_X++)
						{

							Output_Pointer = Output_Buffer_Addreass(Output_Base, Tile_X, Tile_Y, Tile_Byte_X, Tile_Pixel_Y);
							Pixel_Direction = Output_Pixel_Direction(Tile_Byte_X, Tile_Pixel_Y);
							Screen_Pointer = Screen_Buffer_Addreass(Screen_Buffer, Tile_X, Tile_Y, Tile_Byte_X, Tile_Pixel_Y);

							if (Pixel_Direction == 'L') // If pixels are going left to right
							{
								// Insert 8 bytes into output buffer
								for (Pixel = 0; Pixel < 8; Pixel++)
								{
									// Place data into output buffer
									*Output_Pointer = Screen_Pixels_To_Output(Screen_Pointer, Colour_Bit_Buffer);

									// Next pixel
									Output_Pointer++;
									Screen_Pointer += 3; // Jumpus over green and blue colours

								} // end Pixel
							}
							else // If pixels are going right to left
							{
								Output_Pointer += 8;
								// Insert 8 bytes into output buffer
								for (Pixel = 0; Pixel < 8; Pixel++)
								{
									// Place data into output buffer
									*Output_Pointer = Screen_Pixels_To_Output(Screen_Pointer, Colour_Bit_Buffer);

									// Next pixel
									Output_Pointer--;
									Screen_Pointer += 3; // Jumpus over green and blue colours

								} // end Pixel
							}

						} // end Tile_Pixel_X
					}	  // end Tile_Pixel_Y
				}		  // end Tile_X
			}			  // end Tile_Y
		}				  // end Colour_Bit_Buffer
	}					  // End Is 'Screen_Buffer' == NULL

	// Tell hub75_dma_handler() to use this buffer
	// At start of next screen refresh
	if (Current_Output_Buffer == 'A')
	{
		Next_Output_Buffer = 'B';
	}
	else //// Start using Buffer 'A'
	{
		Next_Output_Buffer = 'A';
	}
}

// ***************************************************************************
//   hub75 - DMA
//
// Called when the DMA has finished sending frame buffer to PIO
//
void hub75_dma_handler()
{
	uint32_t Previous_Row;

	Previous_Row = hub75_Current_Scan_Row;

	// Moves pointers to next locations
	hub75_Output_Buffer_Page++;
	if (hub75_Output_Buffer_Page == 0x04) // Have we finished a full frame of data ?
	{
		hub75_Output_Buffer_Page = 0;
		hub75_Current_Scan_Row++;
		if (hub75_Current_Scan_Row >= hub75_ROM->Output_Scan_Lines) // if completed all rows
		{
			hub75_Current_Scan_Row = 0; // Cycle row back to 0
			Current_Output_Buffer = Next_Output_Buffer;
		}
	}

	// Just because the DMA has finished
	// It does not nean the PIO has finished - it will be a short time but best to wait

	// Wait for all data to clear PIO-FIFO buffers
	hub75_wait_stall(hub75_RGB_data_pio, hub75_RGB_data_sm);

	//  Wait for dimming delay to finish
	hub75_wait_stall(hub75_dimming_pio, hub75_dimming_sm);

	// 	Setup correct row drivers  ABCD - E pins on the HUB75 headder

	pio_sm_put_blocking(hub75_row_pio, hub75_row_sm, hub75_ROM->HW_Scan_Mapping[Previous_Row]);

	//  Wait for Row to be set
	hub75_wait_stall(hub75_row_pio, hub75_row_sm);

	// hub75_Latch();

	// Clear DMA interrupt flag
	dma_hw->ints0 = 1u << hub75_dma_channel;

	// Start DMA with correct frame buffer addresses and  Double Buffering page function
	if (Current_Output_Buffer == 'A')
	{
		switch (hub75_Output_Buffer_Page)
		{
		case 0x00:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_A_0 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		case 0x01:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_A_1 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		case 0x02:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_A_2 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		case 0x03:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_A_3 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		}
	}
	else // Use buffer B
	{
		switch (hub75_Output_Buffer_Page)
		{
		case 0x00:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_B_0 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		case 0x01:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_B_1 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		case 0x02:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_B_2 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		case 0x03:
			dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_B_3 + (hub75_Current_Scan_Row * hub75_ROM->Output_Scan_Bytes), true);
			break;
		}
	}

	// Start wait delay, used for dimming
	pio_sm_put_blocking(hub75_dimming_pio, hub75_dimming_sm, Dimming_Delay[hub75_Output_Buffer_Page]);
}

// ***************************************************************************
//  hub75 DMA PIO setup
void hub75_DMA_Init(hub75_t *hub75)
{
	hub75_dma_channel = dma_claim_unused_channel(true);
	dma_channel_config c = dma_channel_get_default_config(hub75_dma_channel);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
	channel_config_set_read_increment(&c, true);
	channel_config_set_dreq(&c, DREQ_PIO0_TX0);

	dma_channel_configure(
		hub75_dma_channel,
		&c,
		&pio0_hw->txf[0],
		NULL,
		hub75_ROM->Output_Scan_Bytes, // Total number of bytes in one scan line for all panels
		false);

	dma_channel_set_irq1_enabled(hub75_dma_channel, true);
	irq_set_exclusive_handler(DMA_IRQ_1, hub75_dma_handler);
	irq_set_enabled(DMA_IRQ_1, true);
}

// ***************************************************************************
//  hub75 - PIO setup
void hub75_PIO_Init(hub75_t *hub75)
{
	uint hub75_rgb_data_offset = pio_add_program(hub75_RGB_data_pio, &hub75_rgb_data_program);
	uint hub75_row_offset = pio_add_program(hub75_row_pio, &hub75_row_program);
	uint hub75_dimming_offset = pio_add_program(hub75_dimming_pio, &hub75_dimming_program);

	// Calculate the PIO Data clock divider
	static const float Data_pio_freq = (HUB75_DATA_CLOCK_HZ * 2);
	float Data_div = (float)clock_get_hz(clk_sys) / Data_pio_freq;

	// Calculate the PIO row clock divider
	static const float Row_pio_freq = (HUB75_DATA_CLOCK_HZ * 2);
	float Row_div = (float)clock_get_hz(clk_sys) / Row_pio_freq;

	// Calculate the PIO dimming clock divider
	static const float Dimming_pio_freq = (HUB75_DIMMING_CLOCK_HZ * 2);
	float Dimming_div = (float)clock_get_hz(clk_sys) / Dimming_pio_freq;

	hub75_RGB_data_sm = pio_claim_unused_sm(hub75_RGB_data_pio, true);
	hub75_row_sm = pio_claim_unused_sm(hub75_row_pio, true);
	hub75_dimming_sm = pio_claim_unused_sm(hub75_dimming_pio, true);

	hub75_rgb_data_program_init(
		hub75_RGB_data_pio,
		hub75_RGB_data_sm,
		hub75_rgb_data_offset,
		HUB75_BASE_RGB_PIN,
		HUB75_CLOCK_PIN,
		Data_div);

	hub75_row_program_init(
		hub75_row_pio,
		hub75_row_sm,
		hub75_row_offset,
		HUB75_BASE_ADDR_PIN,
		HUB75_ADDRESS_LINES,
		HUB75_LATCH_PIN,
		Row_div);

	hub75_dimming_program_init(
		hub75_dimming_pio,
		hub75_dimming_sm,
		hub75_dimming_offset,
		HUB75_OE_PIN,
		Dimming_div);

	// Start PIO SMs
	pio_sm_set_enabled(hub75_RGB_data_pio, hub75_RGB_data_sm, true);
	pio_sm_set_enabled(hub75_row_pio, hub75_row_sm, true);
	pio_sm_set_enabled(hub75_dimming_pio, hub75_dimming_sm, true);
}

// ***************************************************************************
// Get the hub75 driver ready to start displaying images
void Hub75_Init(hub75_t *hub75)
{
	hub75_ROM = hub75;

	// **********************************
	// Internal control stuff
	hub75_Current_Scan_Row = 0;
	hub75_Output_Buffer_Page = 0;

	Current_Output_Buffer = DOUBLE_BUFF_A;
	Next_Output_Buffer = DOUBLE_BUFF_A;

	// Data sent to PIO-sm via DMA buffer A
	hub75_Output_Buffer_A_0 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));
	hub75_Output_Buffer_A_1 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));
	hub75_Output_Buffer_A_2 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));
	hub75_Output_Buffer_A_3 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));

	// Data sent to PIO-sm via DMA buffer B
	hub75_Output_Buffer_B_0 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));
	hub75_Output_Buffer_B_1 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));
	hub75_Output_Buffer_B_2 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));
	hub75_Output_Buffer_B_3 = calloc(hub75_ROM->Output_Bytes, sizeof(uint8_t));

	hub75_PIO_Init(hub75);
	hub75_DMA_Init(hub75);

	// Start DMA.
	dma_hw->ints0 = 1u << hub75_dma_channel; // Clear DMA interrupt flag
	dma_channel_set_read_addr(hub75_dma_channel, hub75_Output_Buffer_A_0, true);
}

// ***************************************************************************
//
// Directaly fill up the output buffer to see the pixel screen order
//
void Hub75_Test_Scan_Buffer(uint32_t Fill_Speed_us)
{
	uint8_t *prt_0;
	uint8_t *prt_1;
	uint8_t *prt_2;
	uint8_t *prt_3;
	uint32_t lp;
	uint32_t counter;
	bool wipe;

	// Safe blank of buffer A and B

	while (1)
	{
		Hub75_Update(NULL);
		sleep_ms(100);
		Hub75_Update(NULL);
		sleep_ms(100);

		counter = 0;
		wipe = false;
		while (!wipe)
		{
			// If driver is currently using buffer A start filling buffer B
			if (Current_Output_Buffer == 'A')
			{
				prt_0 = hub75_Output_Buffer_B_0;
			}
			else // The driver is currently using buffers B then Fill Buffer A
			{
				prt_0 = hub75_Output_Buffer_A_0;
			}

			// Fill up the buffer to counter
			lp = hub75_ROM->Output_Bytes;
			for (lp = 0; lp <= counter; lp++)
			{
				*prt_0++ = 255;
			}

			// Move onto next pixel;
			counter++;
			if (counter == hub75_ROM->Output_Bytes)
			{
				counter = 0;
				wipe = true;
			}

			sleep_ms(100);
			// Swop to using the buffer just filled above
			if (Current_Output_Buffer == 'A')
				Next_Output_Buffer = 'B';
			else
				Next_Output_Buffer = 'A';

			// Wait for a while for the new data to be displayed

		} // end while wipe
	}
}
