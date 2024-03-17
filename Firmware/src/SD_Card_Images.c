// ***************************************************************************
//
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"

// experminting with sd CARDS
#include "sd_card.h"
#include "ff.h"

#include "SD_Card_Images.h"
#include "flash_image_storage.h"
#include "hub75.h"
#include "graphics.h"
#include "BH1730.h"

// ***************************************************************************
// State machine for reading BMP files
typedef enum
{
    FILE_INIT_DRIVER,
    FILE_MOUNT,
    FILE_OPEN,
    FILE_HEADDER,
    FILE_READ_DIB,
    FILE_DIB_PROCESSING,
    FILE_READ_IMAGE_DATA,
    FILE_PROCESS_IMAGE_DATA,
    FILE_CLOSE,
    FILE_UNMOUNT,
    FILE_FAILED

} FileReadStatusType;

#define FILE_BMP_HEADDER_SIZE 14
#define FILE_DIB_HEADDER_SIZE 40

// ***************************************************************************
//
int Read_Image(char *filename, uint16_t Image_Pixel_Width, uint16_t Image_Pixel_Height, uint8_t *Screen_Buffer)
{
    UINT tmp;
    FRESULT fr;
    FATFS fs;
    FIL fil;
    int ret;

    // State machine control
    bool Finished;
    bool Failed;
    uint32_t Expected_Image_Byte_Size = (Image_Pixel_Width * Image_Pixel_Height * 3);
    uint32_t Image_Byte_Line_Length;

    FileReadStatusType Reading_status;

    // DIB image info
    uint32_t Image_Width;
    uint32_t Image_Height;
    uint32_t Color_Planes;
    uint32_t Bits_Per_Pixel;
    uint32_t Compression;
    uint32_t Raw_Bitmap_Size;

    char buf[FILE_DIB_HEADDER_SIZE];

    printf("Reading from file '%s':\r\n", filename);

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
                printf("ERROR: Could not initialize SD card\r\n");
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
            Reading_status = FILE_HEADDER;
            fr = f_open(&fil, filename, FA_READ);
            if (fr != FR_OK)
            {
                printf("ERROR: Could not open file (%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_UNMOUNT;
            }
            break;
        // *********************************
        case FILE_HEADDER: // Read in BMP headder

            Reading_status = FILE_READ_DIB;
            fr = f_read(&fil, buf, FILE_BMP_HEADDER_SIZE, &tmp); // 14 bytes is standered size for BPM headder
            if (fr != FR_OK)
            {
                printf("ERROR: Could not read BMP headder (%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }

            // Check how many bytes were actuly read
            if (tmp != FILE_BMP_HEADDER_SIZE)
            {
                printf("ERROR: Premature EOF(%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }
            else
            {
                // Check for 'BMP' file format
                if ((buf[0] != 'B' || buf[1] != 'M'))
                {
                    printf("ERROR: Invalid file format\r\n");
                    Failed = true;
                    Reading_status = FILE_CLOSE;
                }
            }
            break;
        // *********************************
        case FILE_READ_DIB:

            Reading_status = FILE_DIB_PROCESSING;

            // Get the size of DIB headder using 'Pixel_Array_Offset' - FILE_BMP_HEADDER_SIZE
            uint32_t DIB_Size;

            DIB_Size = buf[0x0A];
            DIB_Size += buf[0x0B] << 8;
            DIB_Size += buf[0x0C] << 16;
            DIB_Size += buf[0x0D] << 24;
            DIB_Size -= FILE_BMP_HEADDER_SIZE;

            // Range check the DIB_size
            if (DIB_Size != FILE_DIB_HEADDER_SIZE)
            {
                printf("ERROR: DIB headder to large(%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }

            // Read DIB headder
            fr = f_read(&fil, buf, DIB_Size, &tmp);
            if (fr != FR_OK)
            {
                printf("ERROR: Could not read DIB headder (%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }

            // Check how many bytes were actuly read
            if (tmp != FILE_DIB_HEADDER_SIZE)
            {
                printf("ERROR: Premature EOF(%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }
            break;
        // *********************************
        case FILE_DIB_PROCESSING: // Process/verify DIB headder

            Reading_status = FILE_READ_IMAGE_DATA;

            // Get image width
            Image_Width = buf[0x04];
            Image_Width += buf[0x05] << 8;
            Image_Width += buf[0x06] << 16;
            Image_Width += buf[0x07] << 24;

            // Get image height
            Image_Height = buf[0x08];
            Image_Height += buf[0x09] << 8;
            Image_Height += buf[0x0A] << 16;
            Image_Height += buf[0x0B] << 24;

            // Get Number of color planes
            Color_Planes = buf[0x0C];
            Color_Planes += buf[0x0D] << 8;

            // Get Number of bits per pixel
            Bits_Per_Pixel = buf[0x0E];
            Bits_Per_Pixel += buf[0x0F] << 8;

            // Get pixel array compression
            Compression = buf[0x10];
            Compression += buf[0x11] << 8;
            Compression += buf[0x12] << 16;
            Compression += buf[0x13] << 24;

            // Get Size of the raw bitmap data (note this can include padding after the actule image data, data is 4 byte aligned data)
            Raw_Bitmap_Size = buf[0x14];
            Raw_Bitmap_Size += buf[0x15] << 8;
            Raw_Bitmap_Size += buf[0x16] << 16;
            Raw_Bitmap_Size += buf[0x17] << 24;

            if ((Image_Width != Image_Pixel_Width) ||
                (Image_Height != Image_Pixel_Height) ||
                (Color_Planes != 1) ||
                (Compression != 0) ||
                (Bits_Per_Pixel != 24))             
            {
                printf("ERROR: Incompatable image (%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }

            break;
        // *********************************
        case FILE_READ_IMAGE_DATA: // Read image pixel data

            Reading_status = FILE_PROCESS_IMAGE_DATA;

            fr = f_read(&fil, Screen_Buffer, Expected_Image_Byte_Size, &tmp);
            if (fr != FR_OK)
            {
                printf("ERROR: Could not read colour data(%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }

            // Check how many bytes were actuly read
            if (tmp != (Image_Pixel_Height * Image_Pixel_Height * (Bits_Per_Pixel >> 3)))
            {
                printf("ERROR: Premature EOF(%d)\r\n", fr);
                Failed = true;
                Reading_status = FILE_CLOSE;
            }

            // All data has been read from the SD card now
            // Finish up by closing file, process the data after
            Finished = true;
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

        } // end switch statment
    }     // end while finished loop

    if (Failed)
    {
        return (1);
    }

    // Flip the BMP Image upside down
    uint8_t *Top;
    uint8_t *Bottom;

    uint32_t Line_Size = Image_Width * 3;
    uint8_t *Backup;
    uint8_t *m_Backup = malloc(Line_Size);
    uint32_t lp;

    Backup = m_Backup;
    Top = Screen_Buffer;                                   // start of Top
    Bottom = Top + (Expected_Image_Byte_Size - Line_Size); // Start of Bottom line

    // Repeat over half the number of image lines
    lp = Image_Height >> 1;
    while (lp--)
    {
        // Swop image lines Over
        memcpy(Backup, Bottom, Line_Size); // Backup Bottom
        memcpy(Bottom, Top, Line_Size);    // Copy from Top to bottom
        memcpy(Top, Backup, Line_Size);    // Restore the bottom line back to top
        Top += Line_Size;
        Bottom -= Line_Size;
    }

    // Free Buffer
    free(m_Backup);

    return (0);
}
