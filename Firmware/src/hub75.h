
#ifndef __HUB75_H_
#define __HUB75_H_

// Stuff used for setting up PIO uints
#define HUB75_ADDRESS_LINES 5					   // A-B-C-D-E  -  Not all are used for every tile type
#define HUB75_BASE_RGB_PIN 0					   // Data pins R1,G1,B1,R2,G2,B2
#define HUB75_LATCH_PIN 12						   // Latch data to outputs
#define HUB75_OE_PIN 13							   // Output enable pin , note is Active high
#define HUB75_CLOCK_PIN 11						   // Clock
#define HUB75_BASE_ADDR_PIN 6					   // Start of Pins A-B-C-D-E
#define HUB75_DATA_CLOCK_HZ 20000000			   // Set A clock of 10Mhz can go upto 20Mhz
#define HUB75_DIMMING_CLOCK_HZ HUB75_DATA_CLOCK_HZ // Same as data clock

// Colour Order
#define COLOUR_ORDER_RGB 0 // Red   : Green : Blue - Most tiles are like this
#define COLOUR_ORDER_RBG 1 // Red   : Blue  : Green
#define COLOUR_ORDER_GRB 2 // Green : Red   : Blue
#define COLOUR_ORDER_GBR 3 // Green : Blue  : Red
#define COLOUR_ORDER_BRG 4 // Blue  : Red   : Green
#define COLOUR_ORDER_BGR 5 // Blue  : Green : Red

//
#define DOUBLE_BUFF_A 'A'
#define DOUBLE_BUFF_B 'B'

// ****************************************************************************************
//
typedef struct
{
	// Values that need to be configered by user
	uint8_t *Pixel_Mapping;		 // Used to map pixels into output buffer
	uint8_t *Pixel_Direction;	 // Used to map pixels into output buffer
	uint32_t Max_Brightness;	 // for 16K pixels @ 10Mhz set to 2500
	uint8_t Colour_Order;		 // RGB, BRG, ect ect .... see defines above
								 //
	uint32_t Tiles_X;			 // Number of Panels across
	uint32_t Tiles_Y;			 // Number of Panels down
	uint8_t Tile_Pixel_Width;	 // For Each Panel in Pixels
	uint8_t Tile_Pixel_Height;	 // For Each Panel in Pixels
								 //
	uint8_t Output_Scan_Lines;	 // The number of lines on eash panel for one HW channel
	uint8_t HW_Scan_Mapping[16]; // Mapping the scan line progress to hardware outputs
								 //
	// Caculated values
	uint8_t Tiles;				// Number of tiles
	uint16_t Tile_Pixels;		// Number of pixels on one panel
	uint16_t Tile_Bytes;		// Memory needed for one panel
								//
	uint8_t Image_Pixel_Width;	// Width in pixels for full image
	uint8_t Image_Pixel_Height; // Height in pixels for full image
	uint16_t Image_Pixels;		// The Max size the hardware can suppot is 16K(16384) pixels
	uint16_t Image_Bytes;		// Memory need for full image
								//
	uint16_t Output_Bytes;		// Total size of output buffer Inc all scan lines
	uint16_t Output_Scan_Bytes; // The Number of bytes sent to screen for each scan line

} hub75_t;

// ****************************************************************************************
//
struct Colour_Order_str
{
	uint8_t Red;
	uint8_t Green;
	uint8_t Blue;
};

// ***************************************************************************
//
void hub75_dma_handler();

void Hub75_Init(hub75_t *hub75);
void Colour_ReMap(uint8_t *Screen_Buffer, uint8_t From_Colour_Order, uint8_t To_Colour_Order);
void Hub75_Set_Brightness(uint16_t Light_Level);

void Hub75_Update(uint8_t *Screen_Buffer);

void Hub75_Test_Scan_Buffer(uint32_t Fill_Speed_us);
uint8_t Screen_Pixels_To_Output(uint8_t *Screen_Pointer, uint8_t Buffer_Count);

#endif
