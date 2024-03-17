// ***************************************************************************
//
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"

// experminting with sd CARDS
#include "sd_card.h"
#include "ff.h"

#include "flash_image_storage.h"
#include "commands.h"
// #include "hub75.h"

// ***************************************************************************
// State machine for reading BMP files
typedef enum
{
    FILE_INIT_DRIVER,
    FILE_MOUNT,
    FILE_OPEN,
    FILE_PROCESS,
    FILE_CLOSE,
    FILE_UNMOUNT,
    FILE_FAILED

} FileConfigType;

// ***************************************************************************
//
uint8_t Delm = ':';

uint8_t *Tokens[] = {
    "Max_Brightness",
    "Colour_Order",
    "Panels_X",
    "Panels_Y",
    "Panel_Width",
    "Panel_Height",
    "Panel_Scan_Liens",
    "HW_Scan_Mapping",
    "Mapping",
    "Direction",
    "Images",
    "None"};

#define TOKEN_NUMBER 12
typedef enum
{
    TOKEN_MAX_BRIGHTNESS,
    TOKEN_COLOUR_ORDER,
    TOKEN_PANELS_X,
    TOKEN_PANELS_Y,
    TOKEN_PANEL_WIDTH,
    TOKEN_PANEL_HEIGHT,
    TOKEN_PANEL_SCAN_LIENS,
    TOKEN_HW_SCAN_MAPPING,
    TOKEN_MAPPING,
    TOKEN_DIRECTION,
    TOKEN_IMAGES,
    TOKEN_NONE
} Tokens_t;

#define CMD_DATA_MODE_NUM 0
#define CMD_DATA_MODE_SCAN 1
#define CMD_DATA_MODE_MAPPING 2
#define CMD_DATA_MODE_DIRECTION 4
#define CMD_DATA_MODE_IMAGES 5
uint8_t Command_Parse_Mode;
uint8_t Command_Parse_Counter;

// ***************************************************************************
// On first boot all config flasg bytes will be 0xFF
// Need to know if the flash has been witten to
//
bool Is_First_Run()
{
    uint32_t lp = CONFIG_HUB75_SIZE;
    uint8_t *addr = (uint8_t *)CONFIG_HUB75_ADDRESS;

    // Loop over all flash config memory
    //
    // if 'lp' gets to 0 then all values nust be 0xFF
    while (lp && *addr == 0xFF)
    {
        lp--;
        addr++;
    }

    if (lp)
        return (0); // Has been changed, ie not first run
    return (1);     // All bytes 0xFF so all bytes 0xFF
}

// ***************************************************************************
//
// Return token number if found at begining of input
//
uint8_t Find_Token(uint8_t *text)
{
    uint8_t lp;

    // Remove spaces and Tabs
    while (*text == ' ' || *text == 9)
        text++;

    for (lp = 0; lp < TOKEN_NUMBER; lp++)
    {
        if (!strncmp(Tokens[lp], text, strlen(Tokens[lp])))
        {
            // Jump over matching string
            text += strlen(Tokens[lp]);

            // Remove spaces and Tabs
            while (*text == ' ' || *text == 9)
                text++;

            // Check for delimiter
            if (*text == ':')
                return (lp); // Sucsess found match
        }
    }
    return (TOKEN_NONE); // Not matched
}

// ***************************************************************************
//
uint32_t Cmd_Value(uint8_t *text)
{
    char *ptr;
    uint32_t value;

    // Move to start of value data
    ptr = strchr(text, ':');

    if (ptr)
    {
        ptr++;
        while (*ptr == ' ' || *ptr == 9) // Remove spaces and Tabs
            ptr++;
        return (atol(ptr));
    }
    return (0x0);
}

// ***************************************************************************
//
uint8_t Cmd_Colour(uint8_t *text)
{
    char *ptr;

    // Move to start of value data
    ptr = strchr(text, ':');

    ptr++;

    // *****************************
    // Look  for RGB or RBG
    if (*ptr == 'R')
    {
        ptr++;
        if (*ptr == 'G') // if RG?
        {
            ptr++;
            if (*ptr == 'B') // if RGB
            {
                return (COLOUR_ORDER_RGB);
            }
            return (COLOUR_ORDER_RGB);
        }
        if (*ptr == 'B') // if RB?
        {
            ptr++;
            if (*ptr++ == 'G') // if RBG
            {
                return (COLOUR_ORDER_RBG);
            }
        }
        return (COLOUR_ORDER_RGB);
    }

    // *****************************
    // Look  for GRB or GBR
    if (*ptr == 'G')
    {
        ptr++;
        if (*ptr == 'R') // if GR?
        {
            ptr++;
            if (*ptr == 'B') // if GRB
            {
                return (COLOUR_ORDER_GRB);
            }
            return (COLOUR_ORDER_RGB); // invalid code
        }
        if (*ptr == 'B') // if GB?
        {
            ptr++;
            if (*ptr == 'R') // if GBR
            {
                return (COLOUR_ORDER_GBR);
            }
        }
        return (COLOUR_ORDER_RGB);
    }

    // *****************************
    // Look  for BRG or BGR
    if (*ptr == 'B')
    {
        ptr++;
        if (*ptr == 'G') // if BG?
        {
            ptr++;
            if (*ptr == 'R') // if BGR
            {
                return (COLOUR_ORDER_BGR);
            }
            return (COLOUR_ORDER_RGB); // invalid code
        }
        if (*ptr == 'R') // if BR?
        {
            ptr++;
            if (*ptr++ == 'G') // if BRG
            {
                return (COLOUR_ORDER_BRG);
            }
        }
        return (COLOUR_ORDER_RGB);
    }
    return (COLOUR_ORDER_RGB);
}

// ***************************************************************************
//
uint32_t Cmd_Mapping(uint8_t *text, uint8_t *Mapping)
{
    uint8_t *Mapping_ptr;

    const char s[2] = ",";

    // Limit the amount of data that can be read
    if (Command_Parse_Counter < 128)
    {
        // Move to correct mapping location
        Mapping_ptr = Mapping + Command_Parse_Counter;

        // Split input at the first ','  , replaces the ',' with a 0x00 (NULL)
        text = strtok(text, s);
        while (*text == ' ' || *text == 9 || *text == '0') // Remove any leading spaces, tabs or 0's
            text++;

        while (text != NULL)
        {
            *Mapping_ptr++ = (uint8_t)atoi(text);
            Command_Parse_Counter++;

            // Split input at the next ','  , replaces the ',' with a 0x00 (NULL)
            text = strtok(NULL, s);
            while (*text == ' ' || *text == 9 || *text == '0') // Remove any leading spaces, tabs or 0's
                text++;
        }
    }
}

// ***************************************************************************
//
uint32_t Cmd_HW_Scan_Mapping(uint8_t *text, uint8_t *Mapping)
{
    uint8_t *Mapping_ptr;

    const char s[2] = ",";

    // Limit the amount of data that can be read
    if (Command_Parse_Counter < 128)
    {
        // Move to correct mapping location
        Mapping_ptr = Mapping + Command_Parse_Counter;

        // Split input at the first ','  , replaces the ',' with a 0x00 (NULL)
        text = strtok(text, s);
        while (*text == ' ' || *text == 9 || *text == '0') // Remove any leading spaces, tabs or 0's
            text++;

        while (text != NULL)
        {

            *Mapping_ptr++ = (uint8_t)atoi(text);
            Command_Parse_Counter++;

            // Split input at the next ','  , replaces the ',' with a 0x00 (NULL)
            text = strtok(NULL, s);
            while (*text == ' ' || *text == 9 || *text == '0') // Remove any leading spaces, tabs or 0's
                text++;
        }
    }
}


// ***************************************************************************
//
uint32_t Cmd_Direction(uint8_t *text, uint8_t *Direction)
{
    uint8_t *Direction_ptr;

    const char s[2] = ",";

    // Limit the amount of data that can be read
    if (Command_Parse_Counter < 128)
    {
        // Move to correct direction location in buffer
        Direction_ptr = Direction + Command_Parse_Counter;

        // Split input at the first ','  , replaces the ',' with a 0x00 (NULL)
        text = strtok(text, s);
        while (*text == ' ' || *text == 9) // Remove any leading spaces or tabs
            text++;

        while (text != NULL)
        {
            // Verify and save
            if (*text == 'R' || *text == 'L')
            {
                *Direction_ptr++ = *text;
                Command_Parse_Counter++;
            }

            // Split input at the next ','  ,replaces the ',' with a 0x00 (NULL)
            text = strtok(NULL, s);
            while (*text == ' ' || *text == 9) // Remove any leading spaces or tabs
                text++;
        }
    }
}

// ***************************************************************************
//
void Command_Parse(hub75_t *hub75, uint8_t *Pixel_Mapping, uint8_t *Dir_Mapping, uint8_t *Image_Delays, uint8_t *Image_Names, uint8_t *text)
{
    switch (Find_Token(text))
    {
    case TOKEN_MAX_BRIGHTNESS:
        hub75->Max_Brightness = Cmd_Value(text);
        Command_Parse_Mode = CMD_DATA_MODE_NUM;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_COLOUR_ORDER:
        hub75->Colour_Order = Cmd_Colour(text);
        Command_Parse_Mode = CMD_DATA_MODE_NUM;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_PANELS_X:
        hub75->Tiles_X = Cmd_Value(text);
        Command_Parse_Mode = CMD_DATA_MODE_NUM;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_PANELS_Y:
        hub75->Tiles_Y = Cmd_Value(text);
        Command_Parse_Mode = CMD_DATA_MODE_NUM;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_PANEL_WIDTH:
        hub75->Tile_Pixel_Width = Cmd_Value(text);
        Command_Parse_Mode = CMD_DATA_MODE_NUM;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_PANEL_HEIGHT:
        hub75->Tile_Pixel_Height = Cmd_Value(text);
        Command_Parse_Mode = CMD_DATA_MODE_NUM;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_PANEL_SCAN_LIENS:
        hub75->Output_Scan_Lines = Cmd_Value(text);
        Command_Parse_Mode = CMD_DATA_MODE_NUM;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_HW_SCAN_MAPPING:
        Command_Parse_Mode = CMD_DATA_MODE_SCAN;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_MAPPING: // Read pixel mapping data
        Command_Parse_Mode = CMD_DATA_MODE_MAPPING;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_DIRECTION: // read pixel byte direction
        Command_Parse_Mode = CMD_DATA_MODE_DIRECTION;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_IMAGES: // Read Image file names
        Command_Parse_Mode = CMD_DATA_MODE_IMAGES;
        Command_Parse_Counter = 0;
        break;
    case TOKEN_NONE:
    default:
        switch (Command_Parse_Mode)
        {
        case CMD_DATA_MODE_SCAN:
            Cmd_HW_Scan_Mapping(text,hub75->HW_Scan_Mapping);
            break;
        case CMD_DATA_MODE_MAPPING:
            Cmd_Mapping(text, Pixel_Mapping);
            break;
        case CMD_DATA_MODE_DIRECTION:
            Cmd_Direction(text, Dir_Mapping);
            break;
        case CMD_DATA_MODE_IMAGES:
            break;
        default:
            Command_Parse_Mode = CMD_DATA_MODE_NUM;
            Command_Parse_Counter = 0;
        };
    }; // end switch find token
}

// ***************************************************************************
// Finish setting up values that will be stored in flash(ROM) after being read
// SD config file
//
void Calculate_Other_hub75_Values(hub75_t *hub75, uint32_t Pixel_Mapping_Address, uint32_t Pixel_Direction_Address)
{
    // **********************************
    // Caculate from ones entered by user
    // Address of pixel mapping data
    hub75->Pixel_Mapping = (uint8_t *)Pixel_Mapping_Address;

    // Address of pixel direction data
    hub75->Pixel_Direction = (uint8_t *)Pixel_Direction_Address;

    // Number of panels
    hub75->Tiles = hub75->Tiles_X * hub75->Tiles_Y;

    // Number of pixels on one panel
    hub75->Tile_Pixels = hub75->Tile_Pixel_Width * hub75->Tile_Pixel_Height;

    // Memory needed for one panel
    hub75->Tile_Bytes = hub75->Tile_Pixels * 3; // 3 = One byte for each of R, G and B values

    // Width in pixels of full image
    hub75->Image_Pixel_Width = hub75->Tile_Pixel_Width * hub75->Tiles_X;

    // Height in pixels of full image
    hub75->Image_Pixel_Height = hub75->Tile_Pixel_Height * hub75->Tiles_Y;

    // The Max size the hardware can suppot is 16K(16384) pixels
    hub75->Image_Pixels = hub75->Image_Pixel_Width * hub75->Image_Pixel_Height;

    // Memory need for full image
    hub75->Image_Bytes = hub75->Image_Pixels * 3; // 3 = One byte for each of R, G and B values

    // Total size of output buffer Inc all scan lines , the 2 is because there are 2 RGB hardware channels
    hub75->Output_Bytes = hub75->Image_Pixels / 2;

    // Number of Bytes needed in output buffer for one panel
    hub75->Output_Scan_Bytes = hub75->Output_Bytes / hub75->Output_Scan_Lines;
}

// ***************************************************************************
//
bool Command_Config_Read() // (bool Before_Init)
{
    UINT tmp;
    FRESULT fr;
    FATFS fs;
    FIL fil;
    int ret;

    // State machine control
    bool Finished;
    bool Failed;

    uint8_t Mode = 0;

    // Buffer to store commands to process
    uint8_t Line_Buff[256];

    hub75_t Ram_hub75;
    uint8_t Ram_Pixel_Mapping[CONFIG_MAPPING_SIZE];
    uint8_t Ram_Pix_Dirs[CONFIG_PIX_DIR_SIZE];
    uint8_t Ram_Image_Delays[CONFIG_DELAY_SIZE];
    uint8_t Ram_Image_Names[CONFIG_NAMES_SIZE];

    FileConfigType Reading_status;

    printf("Reading from file '%s':\r\n", "config.ini");

    Reading_status = FILE_INIT_DRIVER;
    Failed = false;
    Finished = false;
    while (!Finished)
    {
        switch (Reading_status)
        {
        // *********************************
        case FILE_INIT_DRIVER: // Initialize SD card
            Reading_status = FILE_MOUNT;
            if (!sd_init_driver())
            {
                printf("ERROR: Could not initialize SD card\r\n", fr);
                Failed = true;
                Reading_status = FILE_FAILED;
            }
            break;
        // *********************************
        case FILE_MOUNT: // Mount drive
            Reading_status = FILE_OPEN;
            fr = f_mount(&fs, "0:", 1);
            if (fr != FR_OK)
            {
                printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_FAILED;
            }
            break;
        // *********************************
        case FILE_OPEN: // Open file for reading
            Reading_status = FILE_PROCESS;
            fr = f_open(&fil, "config.ini", FA_READ);
            if (fr != FR_OK)
            {
                printf("ERROR: Could not open file (%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_UNMOUNT;
            }
            break;
        // *********************************
        case FILE_PROCESS: // Process every line in file
            while (f_gets(Line_Buff, sizeof(Line_Buff), &fil))
            {
                Command_Parse(&Ram_hub75, Ram_Pixel_Mapping, Ram_Pix_Dirs, Ram_Image_Delays, Ram_Image_Names, Line_Buff);
            }
            // Finished
            Reading_status = FILE_CLOSE;
            break;

        // *********************************
        case FILE_CLOSE: // Close file
            Reading_status = FILE_UNMOUNT;
            fr = f_close(&fil);
            if (fr != FR_OK)
            {
                printf("ERROR: Could not close file (%d)\r\n", fr);
                Failed = true;
            }
            break;
        // *********************************
        case FILE_UNMOUNT: // Unmount drive
            f_unmount("0:");
            Finished = true;
            break;

            // *********************************
        case FILE_FAILED:
            Failed = true;
            Finished = true;
            break;

        }; // end switch statment
    }      // end while finished loop

    if (!Failed) // Save to flash in no errors
    {
        Calculate_Other_hub75_Values(&Ram_hub75, CONFIG_MAPPING_ADDRESS, CONFIG_PIX_DIR_ADDRESS);
        Flash_Config_Erase();
        Flash_Config_Save(CONFIG_HUB75_OFFSET, (uint8_t *)&Ram_hub75);
        Flash_Config_Save(CONFIG_MAPPING_OFFSET, Ram_Pixel_Mapping);
        Flash_Config_Save(CONFIG_PIX_DIR_OFFSET, Ram_Pix_Dirs);
    }

    return (Failed);
}
