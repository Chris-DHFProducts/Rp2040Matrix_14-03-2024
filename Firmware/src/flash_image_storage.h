#ifndef __FLASH_IMAGE_STORAGE__
#define __FLASH_IMAGE_STORAGE__

#include "hub75.h"
#include "hardware/flash.h"

// *****************************************************************************
//
//  Flash ROM access for timer data
//
// Found example at https://kevinboone.me/picoflash.html?i=2
//
// The Matrix board has has 16Mb of flash ROM
//
// 16 Megabytes =    16 * 1024 = 16,384 Kilobytes or 16,777,216 bytes
//
// #include flash.h
//
// The SDK defines a constant for the start of flash in the memory map --
//     it is XIP_BASE, with a value of 0x10000000
//
// Erasing data from flash   'flash_range_erase (offset, size);'
//   offset must be a multiple of the "sector size",
//   size  must also be a multiple of the "sector size",
//   which is defined as the constant FLASH_SECTOR_SIZE,
//   with a value of 4kB.

// *****************************************************************************
//
// W25Q128JVSIQ
//
/*
#define FLASH_MBITS  64//128                 // Number of M bits  in storage device
#define FLASH_MBYTES (FLASH_MBITS/8)     // Number of M bytes in storage device
#define FLASH_KBYTES (FLASH_MBYTES*1024) // Number of K bytes in storage device
#define FLASH_BYTES  (FLASH_KBYTES*1024) // Number of   bytes in storage device

// *****************************************************************************
//
#define FLASH_SIZE    FLASH_BYTES
#define FLASH_END     (XIP_BASE + FLASH_SIZE) // This is one more byte then the last
*/

#define FLASH_SIZE (16 * 1024 * 1024) // PICO_FLASH_SIZE_BYTES //((2048*1024)*2) // 2Mb  // 16,777,216
#define FLASH_END (XIP_BASE + FLASH_SIZE)

// The timer data will use the last 4K of Flash ROM
#define FLASH_DATA_OFFSET (FLASH_SIZE - FLASH_SECTOR_SIZE)
#define FLASH_DATA_ADDR (FLASH_END - FLASH_SECTOR_SIZE)

// *****************************************************************************
//
#define IMAGE_PIXEL_WIDTH 128
#define IMAGE_PIXEL_HEIGHT 128
#define IMAGE_SIZE (IMAGE_PIXEL_WIDTH * IMAGE_PIXEL_HEIGHT * 3) // IMAGE_SIZE must be a mutiaple of FLASH_SECTOR_SIZE(4K)
#define IMAGE_NUMBER 256                                        // MAX number of images the max value is 256 = 12Mb of flash !!!

#define IMAGE_ABSOLUTE_ADDRESS (FLASH_END - (IMAGE_SIZE * IMAGE_NUMBER)) // Absolute address in full memory map
#define IMAGE_OFFSET_ADDRESS (FLASH_SIZE - (IMAGE_SIZE * IMAGE_NUMBER))  // Offset address in flash, first byte in flash = address 0

//
// Tile configeration memory is broken into '16 * FLASH_PAGE_SIZE(256b)' chunks
//    that fit into one FLASH_SECTOR_SIZE (4K)
//
// Data for hub74 parameters
#define CONFIG_HUB75_OFFSET (IMAGE_OFFSET_ADDRESS - FLASH_SECTOR_SIZE)
#define CONFIG_HUB75_ADDRESS (IMAGE_ABSOLUTE_ADDRESS - FLASH_SECTOR_SIZE)
#define CONFIG_HUB75_SIZE (FLASH_PAGE_SIZE)

// How each set of 8 pixels are mapped to output buffers
#define CONFIG_MAPPING_OFFSET (CONFIG_HUB75_OFFSET + CONFIG_HUB75_SIZE)
#define CONFIG_MAPPING_ADDRESS (CONFIG_HUB75_ADDRESS + CONFIG_HUB75_SIZE)
#define CONFIG_MAPPING_SIZE (FLASH_PAGE_SIZE)

// The bit direction for each 8 pixel group LTR = 1, RTL = 0
#define CONFIG_PIX_DIR_OFFSET (CONFIG_MAPPING_OFFSET + CONFIG_MAPPING_SIZE)
#define CONFIG_PIX_DIR_ADDRESS (CONFIG_MAPPING_ADDRESS + CONFIG_MAPPING_SIZE)
#define CONFIG_PIX_DIR_SIZE FLASH_PAGE_SIZE

// How long to show each image in 100 * ms , eg 5 = 500ms ( 1/2 sec)
//     MAX = 255 = 255 * 100 = 25500ms  or 25.5s
//
#define CONFIG_DELAY_OFFSET (CONFIG_PIX_DIR_OFFSET + CONFIG_PIX_DIR_SIZE)
#define CONFIG_DELAY_ADDRESS (CONFIG_PIX_DIR_ADDRESS + CONFIG_PIX_DIR_SIZE)
#define CONFIG_DELAY_SIZE FLASH_PAGE_SIZE

// The file names for each of the 256 possiable images
//
// Image name format must be xxxxxxxx.BMP  ( MAX 12 chars long )
//
#define CONFIG_NAMES_OFFSET (CONFIG_DELAY_OFFSET + CONFIG_DELAY_SIZE)
#define CONFIG_NAMES_ADDRESS (CONFIG_DELAY_ADDRESS + CONFIG_DELAY_SIZE)
#define CONFIG_NAMES_SIZE (FLASH_PAGE_SIZE * 12)

// *****************************************************************************
//
int Flash_Image_Retrieve(uint8_t Image_Number, uint8_t *RAM_address, bool Debug);
int Flash_Image_Store(uint8_t Image_Number, uint8_t *RAM_address, bool Debug);
uint8_t *Flash_Image_Address(uint8_t Image_Number, bool Debug);
void Flash_Erase_All(void);

void Flash_Config_Erase(void);
void Flash_Config_Save(uint32_t Buffer, uint8_t *Buffer_Address);


// void Flash_Config_Retrieve(uint8_t *Buffer);
// void Flash_Config_Save(uint8_t *Buffer,uint32_t bytes);

#endif //__FLASH_IMAGE_STORAGE__