// ***************************************************************************
//
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
// #include "hardware/i2c.h"
#include "hardware/irq.h"

#include "interfaces.h" // Hardware pins / ports /  ext....
#include "hub75.h"
#include "inputs.h"

#include "i2cPicoBusScan.h"
#include "BH1730.h"
#include "ads1x1x.h"
#include "graphics.h"

#include "CIE1931.h"
#include "Average.h"

#include "flash_image_storage.h"
#include "SD_Card_Images.h"
#include "commands.h"

// ***************************************************************************
//
#define DEBUG_MAIN_HUB75 false

// ***************************************************************************
//
#define MENU_EXIT 0x00

#define MENU_SHOW_SD_IMAGES 0x01	// Show images on SD card do not copy to flash
#define MENU_FLASH_SHOW_IMAGES 0x02 // Show Flash Images
#define MENU_LOAD_FLASH 0x04		// Copy Images From SD to Flash
#define MENU_CONFIG 0x06			// Load Config from SD
#define MENU_TESTING_PIXEL 0x08		// Pixel Test sequance
#define MENU_CLEAR_FLASH 0x0C		// Erase the flash memory
#define MENU_SCAN_TEST 0x0F			// Used to caculate mapping for new tiles

// ***************************************************************************
// Setup pointers for configeration data
//
// All will point to Flash memory
//
hub75_t *hub75 = (hub75_t *)CONFIG_HUB75_ADDRESS;
uint16_t *Pixel_Mapping = (uint16_t *)CONFIG_MAPPING_ADDRESS;
uint8_t *Pix_Dirs = (uint8_t *)CONFIG_PIX_DIR_ADDRESS;
uint8_t *Image_Delays = (uint8_t *)CONFIG_DELAY_ADDRESS;
uint8_t *Image_Names = (uint8_t *)CONFIG_NAMES_ADDRESS;

ADS1x1x_config_t Ads1115;
bool Brightneess_HW_Input;

bool Core_One_Running_Flag = false;

uint16_t Found_Images = 0;

// ***************************************************************************
//
bool Has_Core_One_Been_Stated()
{
	return (Core_One_Running_Flag);
}

// ***************************************************************************
//
// Core 1 Main Code
void core1_entry()
{

	// Initialize the current core such that it can be a "victim" of lockout (i.e. forced to pause in a known state by the other core)
	// This is needed while writing to Flash memory
	multicore_lockout_victim_init();

	// Start display driver
	Hub75_Init(hub75);
	Core_One_Running_Flag = true;


	// Do nothing as it's all done with IRQ's DMA's and PIO's
	while (1)
		tight_loop_contents();
}

// ***************************************************************************
//
int16_t Read_ADC_Brightness(bool Test_Input, uint32_t uS_Delay, uint32_t Max_LDR)
{
	int16_t result;

	if (Test_Input)
	{
		// Read the Switched ADC input level
		ADS1x1x_init(&Ads1115, ADS1115, ADS1x1x_I2C_ADDRESS_ADDR_TO_GND, MUX_SINGLE_2, PGA_4096);
		ADS1x1x_start_conversion(&Ads1115);
		sleep_us(uS_Delay);
		result = ADS1x1x_read(&Ads1115);
		if (result < 100) // if ADC switch closed we need to read POT input
		{
			ADS1x1x_init(&Ads1115, ADS1115, ADS1x1x_I2C_ADDRESS_ADDR_TO_GND, MUX_SINGLE_1, PGA_4096);
			ADS1x1x_start_conversion(&Ads1115);
			Brightneess_HW_Input = true;
			sleep_us(uS_Delay);
			result = ADS1x1x_read(&Ads1115);
		}
		else // Using LRD input
		{
			ADS1x1x_init(&Ads1115, ADS1115, ADS1x1x_I2C_ADDRESS_ADDR_TO_GND, MUX_SINGLE_0, PGA_4096);
			ADS1x1x_start_conversion(&Ads1115);
			Brightneess_HW_Input = false;
			sleep_us(uS_Delay);
			result = Max_LDR - ADS1x1x_read(&Ads1115);
			if (result < 0)
				result = 0;
		}
	}
	else // Just read the current input
	{
		ADS1x1x_start_conversion(&Ads1115);
		sleep_us(uS_Delay);
		result = ADS1x1x_read(&Ads1115);
		if (!Brightneess_HW_Input)
			result = Max_LDR - result;

		if (result < 0)
			result = 0;
	}
	return (result);
}

// ***************************************************************************
// if Image_Number < 0 then do not store in flash
uint8_t Read_SD_Card_Image(uint8_t *Image_Name, int16_t Image_Number, uint32_t delay_s, uint8_t *Screen_Buffer)
{
	uint16_t lp;
	bool Read_Image_Ok = 1;

	g_Clear(COLOUR_BLACK);
	g_Set_Ink_Colour(COLOUR_RED);

	// Load Image to screen if possiable from SD card
	Hub75_Update(NULL);
	if (Read_Image(Image_Name, hub75->Image_Pixel_Width, hub75->Image_Pixel_Width, Screen_Buffer))
	{
		g_String(0, 0, "UNABLE TO READ SD IMAGE");
		g_String(0, 9, Image_Name);
	}
	else // IF found and loaded sd card image into 'Screen_Buffer'
	{
		// Adjust fix the colour order, needed because of BMP files colour order is different
		Colour_ReMap(Screen_Buffer, COLOUR_ORDER_BGR, COLOUR_ORDER_RGB);

		if (Image_Number >= 0) // Don't Store in flash option
		{
			if (Flash_Image_Store(Image_Number, Screen_Buffer, DEBUG_MAIN_HUB75) != 0)
			{
				g_String(0, 0, Image_Name);
				g_String(0, 9, "FLASH SAVE ERROR");
			}
			else
				Read_Image_Ok = 0;
		}
		else // SD read and Flash write worked OK
			Read_Image_Ok = 0;
	}

	// Display new loaded image
	Hub75_Update(Screen_Buffer);

	// Is Error display for longer
	if (Read_Image_Ok)
		delay_s = 3000;

	// Wait for a bit so user can see Image/Info
	lp = delay_s;
	while (lp--)
	{
		// Set Display brightness
		Hub75_Set_Brightness(CIE1931(Average(ADS1x1x_read(&Ads1115))));

		// Restart ADC for next value
		ADS1x1x_start_conversion(&Ads1115);
		sleep_ms(1); // 1sec
	}

	return (Read_Image_Ok);
}

// ***************************************************************************
//
void Display_All_SD_Images(uint32_t delay_s, uint8_t *Screen_Buffer)
{
	uint16_t Image;
	uint16_t lp;
	uint16_t number_of_Image = IMAGE_NUMBER;
	uint8_t FileName[14];
	bool exit = false;

	while (!exit)
	{
		for (Image = 0; Image < number_of_Image; Image++) // Don't try and do more then System Max Images
		{
			sprintf(FileName, "%04d.bmp", Image);
			if (Read_SD_Card_Image(FileName, -1, delay_s, Screen_Buffer))
			{
				Image = IMAGE_NUMBER; // Stop reading if no more iamges
			}
			else // All was ok
			{
				if (Input_Test_Switch())
				{
					sleep_ms(300);
					exit = true;
					Image = number_of_Image;
				}
			}
		}
	}

	g_Clear(COLOUR_BLACK);
	Hub75_Update(Screen_Buffer);
}

// ***************************************************************************
//
void Display_All_Flash_Images(uint32_t delay_s, uint16_t number_of_Image, uint8_t *Screen_Buffer)
{
	uint16_t Image;
	uint16_t lp;
	uint8_t *Buffer;
	bool exit = false;

	while (!exit)
	{
		for (Image = 16; Image < number_of_Image; Image++)
		{
			Buffer = Flash_Image_Address(Image, DEBUG_MAIN_HUB75);
			Hub75_Update(Buffer);

			// Wait for a bit so user can see Image
			lp = delay_s;
			while (lp--)
			{
				// Set Display brightness
				Hub75_Set_Brightness(CIE1931(Average(ADS1x1x_read(&Ads1115))));

				// Restart ADC for next value
				ADS1x1x_start_conversion(&Ads1115);
				Image++;
				if (Input_Test_Switch())
				{
					sleep_ms(300);
					exit = true;
					Image = number_of_Image;
				}
			}
		}
	}

	g_Clear(COLOUR_BLACK);
	Hub75_Update(Screen_Buffer);
}

// ***************************************************************************
//
void Didplay_Pixel_Testing(uint8_t *Screen_Buffer, uint32_t delay)
{
	g_Clear(COLOUR_RED);
	Hub75_Update(Screen_Buffer);
	sleep_ms(delay);

	g_Clear(COLOUR_GREEN);
	Hub75_Update(Screen_Buffer);
	sleep_ms(delay);

	g_Clear(COLOUR_BLUE);
	Hub75_Update(Screen_Buffer);
	sleep_ms(delay);

	g_Clear(COLOUR_BLACK);
	Hub75_Update(Screen_Buffer);
	sleep_ms(delay);

	g_Clear(COLOUR_WHITE);
	Hub75_Update(Screen_Buffer);
	sleep_ms(delay);

	g_Clear(COLOUR_BLACK);
	Hub75_Update(Screen_Buffer);
}

// ***************************************************************************
//
// Return the number of Images found
uint16_t Read_All_SD_Images_And_Store(uint8_t *Screen_Buffer, uint32_t delay_s)
{
	uint16_t Image;
	uint8_t FileName[14];

	Found_Images = 0;
	for (Image = 0; Image < IMAGE_NUMBER; Image++)
	{
		sprintf(FileName, "%04d.bmp", Image);
		if (Read_SD_Card_Image(FileName, Image, delay_s, Screen_Buffer))
		{
			Found_Images = Image;
			Image = IMAGE_NUMBER; // Stop reading if no more iamges
		}
	}

	g_Clear(COLOUR_BLACK);
	Hub75_Update(Screen_Buffer);

	return (Found_Images);
}

// ***************************************************************************
//
void Display_Flash_Erase_All(uint8_t *Screen_Buffer)
{
	uint8_t x;
	bool NotFinished = true;

	g_Clear(COLOUR_BLACK);
	g_Set_Ink_Colour(COLOUR_RED);
	g_String(0, 0, "ERASE FLASH IMAGES");
	g_String(0, 9, "PRESS AND HOLD");
	g_String(0, 18, "TEST");

	// Draw empty boxes
	for (x = 0; x < 8; x++)
	{
		// Outer White box
		g_Set_Ink_Colour(COLOUR_WHITE);
		g_Square((x * 12) + 20, 27, 7, 7);

		g_Set_Ink_Colour(COLOUR_BLACK);
		g_Square((x * 12) + 21, 27 + 1, 5, 5);
	}
	Hub75_Update(Screen_Buffer);
	sleep_ms(500);

	// Wait for user to press the test button
	while (!(Input_Test_Switch()))
		;

	x = 0;
	g_Set_Ink_Colour(COLOUR_GREEN);

	while (NotFinished)
	{
		g_Square((x * 12) + 21, 27 + 1, 5, 5);
		Hub75_Update(Screen_Buffer);
		sleep_ms(1000);

		if (Input_Test_Switch())
		{
			x++;
			if (x == 8)
			{
				g_Clear(COLOUR_BLACK);
				g_Set_Ink_Colour(COLOUR_AMBER);
				g_String(0, 0, "ERASEING FLASH IMAGES");
				g_String(0, 9, "DISPLAY WILL BLANK");
				g_String(0, 18, "FOR APROX 1 MIN");

				g_Set_Ink_Colour(COLOUR_BLUE);
				g_String(0, 31, "PRESS TO START");
				Hub75_Update(Screen_Buffer);
				sleep_ms(500);

				// Wait for user to press the test button
				while (!(Input_Test_Switch()))
					;

				Flash_Erase_All();

				g_Clear(COLOUR_BLACK);
				g_Set_Ink_Colour(COLOUR_GREEN);
				g_String(0, 0, "FINISHED ERASEING");
				g_String(0, 9, "FLASH IMAGES");

				g_Set_Ink_Colour(COLOUR_BLUE);
				g_String(0, 20, "PRESS TO EXIT");
				Hub75_Update(Screen_Buffer);
				sleep_ms(500);

				while (!(Input_Test_Switch()))
					;
				NotFinished = false;
			}
		}
		else
			NotFinished = false;
	}

	if (x != 8) // User did not keep test switch pressed
	{
		g_Clear(COLOUR_BLACK);
		g_Set_Ink_Colour(COLOUR_AMBER);
		g_String(0, 0, "FLASH NOT ERASED");
		Hub75_Update(Screen_Buffer);
		sleep_ms(3000);
	}

	// Clean screen and exit
	g_Clear(COLOUR_BLACK);
	Hub75_Update(Screen_Buffer);
	sleep_ms(500);
}

// ***************************************************************************
//
void Test_Image_128x128(uint8_t *Screen_Buffer)
{
	bool Exit = false;
	uint8_t x;
	uint8_t y;
	uint8_t lpx;
	uint8_t lpy;
	uint32_t colour;
	uint32_t next_colour;
	uint32_t colours[3] = {0x00000010, 0x00001000, 0x00100000};
	uint32_t light_Raw_ADC;
	uint32_t light_Display;
	float percent;

	g_Set_Background_Colour(COLOUR_BLACK); // Text Background colour
	g_Clear(COLOUR_BLACK);

	LED_Toggle(0, 500);
	while (!Exit)
	{
		// Display mode set by switches
		g_Set_Ink_Colour(COLOUR_CYAN);
		switch (Input_Switch())
		{
		case MENU_LOAD_FLASH:
			g_String(2, 2, "LOAD ");
			break;

		case MENU_SHOW_SD_IMAGES:
			g_String(2, 2, "SD   ");
			break;

		case MENU_FLASH_SHOW_IMAGES:
			g_String(2, 2, "FLASH");
			break;

		case MENU_TESTING_PIXEL:
			g_String(2, 2, "TEST ");
			break;

		case MENU_CLEAR_FLASH:
			g_String(2, 2, "CLEAR");
			break;

		case MENU_SCAN_TEST:
			g_String(2, 2, "SCAN ");
			break;

		case MENU_CONFIG:
			g_String(2, 2, "INST ");
			break;

		case MENU_EXIT:
			g_String(2, 2, "EXIT ");
			break;

		default:
			g_String(2, 2, "-----");
		}

		// Display brightness as a percentage
		light_Raw_ADC = Read_ADC_Brightness(true, 50000, 26344);
		light_Display = Average(light_Raw_ADC);
		percent = (100.0 / (26589.0)) * light_Display;
		if (Brightneess_HW_Input)
			g_Set_Ink_Colour(COLOUR_GREEN);
		else
			g_Set_Ink_Colour(COLOUR_MAGENTA);
		g_Float(1, 10, percent);

		Hub75_Set_Brightness(CIE1931(Average(light_Raw_ADC)));

		// Switch inputs
		for (x = 0; x < 4; x++)
		{
			// Outer White box
			g_Set_Ink_Colour(COLOUR_GRAY);
			g_Square((x * 3) + 2, 18, 4, 4);

			if ((Input_Switch() & (1 << x)))
				g_Set_Ink_Colour(COLOUR_RED);
			else
				g_Set_Ink_Colour(COLOUR_BLACK);

			// Middle
			g_Square((x * 3) + 3, 19, 2, 2);
		}

		// Opto inputs
		for (x = 0; x < 4; x++)
		{
			g_Set_Ink_Colour(COLOUR_GRAY);
			g_Square((x * 3) + 2, 21, 4, 4);

			if ((Input_Opto() & (1 << x)))
				g_Set_Ink_Colour(COLOUR_RED);
			else
				g_Set_Ink_Colour(COLOUR_BLACK);

			g_Square((x * 3) + 3, 22, 2, 2);
		}

		// Test switch
		g_Set_Ink_Colour(COLOUR_GRAY);
		g_Square(26, 18, 4, 4);

		if (Input_Test_Switch())
			g_Set_Ink_Colour(COLOUR_MAGENTA);
		else
			g_Set_Ink_Colour(COLOUR_BLACK);

		g_Square(27, 19, 2, 2);

		g_Set_Ink_Colour(COLOUR_GRAY);
		g_Square(26, 21, 4, 4);

		if (Brightneess_HW_Input)
			g_Set_Ink_Colour(COLOUR_GREEN);
		else
			g_Set_Ink_Colour(COLOUR_MAGENTA);

		g_Square(27, 22, 2, 2);

		// Colour Grid
		for (lpy = 0; lpy < 3; lpy++)
		{
			next_colour = colours[lpy];
			colour = next_colour;
			for (lpx = 0; lpx < 15; lpx++)
			{
				g_Set_Ink_Colour(colour);
				g_Square((lpx * 2) + 2, (lpy * 2) + 25, 2, 2);
				colour += next_colour;
			}
		}

		// Screen Borbers
		for (x = 0; x <= hub75->Image_Pixel_Width; x++)
		{
			g_Plot(x, 0, COLOUR_YELLOW);
			g_Plot(x, hub75->Image_Pixel_Height - 1, COLOUR_YELLOW);
		}
		for (x = 0; x <= hub75->Image_Pixel_Height; x++)
		{
			g_Plot(0, x, COLOUR_YELLOW);
			g_Plot(hub75->Image_Pixel_Width - 1, x, COLOUR_YELLOW);
		}
		// Alt Screen and Board test functions
		// Do a test of output buffers
		// Use test swwitch to select more test modes
		if (Input_Test_Switch())
		{
			sleep_ms(300); // Switch de-bounce time
			switch (Input_Switch())
			{

			case MENU_LOAD_FLASH: // Reads images Off SD card and save in flash
				Read_All_SD_Images_And_Store(Screen_Buffer, 1);
				break;

			case MENU_CONFIG:		   // Reads images Off SD card and save in flash
				Command_Config_Read(); //	Flash_Config_Save(hub75,Pixel_Mapping, sizeof(Pixel_Mapping));
				break;

			case MENU_SHOW_SD_IMAGES: // Reads images Off SD card but does not save to flash, all 256 images are loaded in sequance
				Display_All_SD_Images(100, Screen_Buffer);
				g_Clear(COLOUR_BLACK);
				break;

			case MENU_FLASH_SHOW_IMAGES:
				Display_All_Flash_Images(1, IMAGE_NUMBER, Screen_Buffer);
				break;

			case MENU_TESTING_PIXEL:
				Didplay_Pixel_Testing(Screen_Buffer, 3000);
				break;

			case MENU_CLEAR_FLASH:
				Display_Flash_Erase_All(Screen_Buffer);
				break;

			case MENU_SCAN_TEST:
				Hub75_Test_Scan_Buffer(1000000);
				g_Clear(COLOUR_BLACK);
				break;

			case MENU_EXIT:
				Exit = true;
				break;

			} // end switch (Input_Switch())
		}	  // End Test switch

		// Display New Image
		Hub75_Update(Screen_Buffer);

		LED_Toggle(-1, -1);
		sleep_ms(250);

	} // While NOT exit

	// Clear screen
	g_Clear(COLOUR_BLACK);
	Hub75_Update(Screen_Buffer);
	sleep_ms(250);
}

// ***************************************************************************
//
// Allow the configeration memory to be erased by pressing TEST and having all switched ON
// Will require a power/reset to continue
//
void Config_Load()
{
	bool Loaded = false;

	if (Is_First_Run())
	{
		// Flash LED to indicate a first run power up
		LED_Pluse(50, 10);
		sleep_ms(500);
		LED_Pluse(50, 10);
		sleep_ms(500);
		LED_Pluse(50, 10);
		sleep_ms(500);
		while (!Loaded)
		{
			// Wait for user to press switch
			if ((Input_Test_Switch() && Input_Switch() == MENU_SCAN_TEST))
			{
				sleep_ms(250); // Switch de-bounce time

				if (Command_Config_Read()) // Try and read config data from SD card and store in flash
				{
					// Failed to load confifgeration , wait for reset
					while (1)
					{
						LED_Pluse(50, 10);
						sleep_ms(250);
					}
				}
				else
					Loaded = true; // Will cause exit with loaded config
			}
			else // Flash LED until user presses switch
			{
				LED_Pluse(50, 10); // Confifgeration not loaded
				sleep_ms(500);
			}
		}
		return;
	}

	if (Input_Test_Switch())
	{
		if (Input_Switch() == MENU_SCAN_TEST)
		{
			// Flash LED to indicate loading a new config
			LED_Pluse(50, 10);
			sleep_ms(500);
			LED_Pluse(50, 10);
			sleep_ms(500);
			LED_Pluse(50, 10);
			sleep_ms(500);

			if (Command_Config_Read()) // Try and read config data from SD card and store in flash
			{
				// Failed to load confifgeration , wait for reset
				while (1)
				{
					LED_Pluse(50, 10);
					sleep_ms(250);
				}
			}
		}
	}

	return;
}

// ***************************************************************************
//
// Running on Core 0
//
int main()
{
	uint8_t current_Opto;

	// Serial port output for PICO  SDK
	stdio_init_all();

	// User Switch and Opto input setup
	Input_Setup();

	// **********************************************
	// On first boot the config must be loaded from SD config file
	// or the card will not function
	Config_Load(); //  ***** Will not exit if user is pressing Test switch and all switched 1-4 all on
				   // Requires Power reset

	// **********************************************
	// Create A screen buffer to hold image data in sRAM, also used as buffer for reading SC-Card
	// Must be called after Panel hardware configeration setup
	// Is never freed
	uint8_t *Screen_Buffer = malloc(IMAGE_SIZE); // Use the cards MAX image size

	// **********************************************
	// Setup graphics for drawing
	g_Init(Screen_Buffer, hub75->Image_Pixel_Width, hub75->Image_Pixel_Height);

	// Make sure screen and output buffers are blank
	g_Clear(COLOUR_BLACK);

	// Stats code running on core 1
	multicore_launch_core1(core1_entry);

	// ****************************************************************************************
	//
	// From this point on Core 1 is driving the display
	//
	// ****************************************************************************************

	// I2C Setup
	i2c_bus_setup_scan(I2C_PORT, I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_CLOCK_SPEED);

	// Select the correct analogue input / POT or LRD ?

	// Anagogue Input Steup
	Average_Init(Read_ADC_Brightness(true, 50000, 26344), 8); // 6 Repersents the smoothing ( bigger = slower but smother )
	CIE1931_Init(26578, 4, hub75->Max_Brightness);			  // Issue with using floting point numbers

	// Display Logo Text
	Hub75_Set_Brightness(CIE1931(Average(Read_ADC_Brightness(true, 50000, 26344))));
	g_Clear(COLOUR_BLACK);
	g_Set_Ink_Colour(COLOUR_WHITE);
	//	g_String(0, 0, "DHF-PRODUCTS");
	//	g_String(0, 9, "(c) 2024  ");
	Hub75_Update(Screen_Buffer);
	// sleep_ms(2000);

	LED_Toggle(0, 20000);
	while (1)
	{
		// **********************************************
		//
		if (Input_Switch() != 0 && Input_Test_Switch())
		{
			sleep_ms(300); // Switch de-bounce time
			Test_Image_128x128(Screen_Buffer);
			current_Opto = !current_Opto;
		}

		// Only update if Opto inputs have changed
		if (current_Opto != Input_Opto())
		{
			current_Opto = Input_Opto();

			// Display image from flash memory with user input
			Hub75_Update(Flash_Image_Address(current_Opto, DEBUG_MAIN_HUB75));
			sleep_ms(500); // Wait for 1/2 sec
		}
		Hub75_Set_Brightness(CIE1931(Average(Read_ADC_Brightness(false, 50000, 26344))));

		LED_Toggle(-1, -1);
		sleep_ms(50);
	}
}
