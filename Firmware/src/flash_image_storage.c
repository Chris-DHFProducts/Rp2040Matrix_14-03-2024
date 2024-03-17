// ***************************************************************************
//
#include "stdio.h"
#include "string.h"

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#include "main.h"
#include "commands.h"
#include "flash_image_storage.h"

void Flash_Config_Erase(void)
{
    uint32_t ints;

    // Setup to enable safe write to fmash memory
    if (Has_Core_One_Been_Stated())
        multicore_lockout_start_blocking();

    ints = save_and_disable_interrupts();

    // Erase the full 4K memory block
    flash_range_erase(CONFIG_HUB75_OFFSET, FLASH_SECTOR_SIZE);

    // Finished writing to flash
    restore_interrupts(ints);

    if (Has_Core_One_Been_Stated())
        multicore_lockout_end_blocking();
}

// *****************************************************************************
//
// The config flash memory uses 4k block directly below the Images
//
void Flash_Config_Save(uint32_t Buffer, uint8_t *Buffer_Address)
{
    uint32_t ints;

    // Setup to enable safe write to fmash memory
    if (Has_Core_One_Been_Stated())
        multicore_lockout_start_blocking();
    ints = save_and_disable_interrupts();

    switch (Buffer)
    {
    case CONFIG_HUB75_OFFSET: // HUB75 Tile configeration
        flash_range_program(CONFIG_HUB75_OFFSET, (const uint8_t *)Buffer_Address, CONFIG_HUB75_SIZE);
        break;

    case CONFIG_MAPPING_OFFSET: // Tile Pixel Mapping
        flash_range_program(CONFIG_MAPPING_OFFSET, (const uint8_t *)Buffer_Address, CONFIG_MAPPING_SIZE);
        break;

    case CONFIG_PIX_DIR_OFFSET: // Tile Pixel Mapping direction LTR or RTL
        flash_range_program(CONFIG_PIX_DIR_OFFSET, (const uint8_t *)Buffer_Address, CONFIG_PIX_DIR_SIZE);
        break;

    case CONFIG_DELAY_OFFSET: // Time to show image in 100ms steps ,  5 = 500ms or 1/2 sec
        flash_range_program(CONFIG_DELAY_OFFSET, (const uint8_t *)Buffer_Address, CONFIG_DELAY_SIZE);
        break;

    case CONFIG_NAMES_OFFSET:
        // SD card Image manes, must be 8 chars + 0.bmp  "00000000.bmp"
        flash_range_program(CONFIG_NAMES_OFFSET, (const uint8_t *)Buffer_Address, CONFIG_NAMES_SIZE);
        break;
    }
    // Finished writing to flash
    restore_interrupts(ints);
    if (Has_Core_One_Been_Stated())
        multicore_lockout_end_blocking();
}

// *****************************************************************************
// Copy image from flash into RAM
int Flash_Image_Retrieve(uint8_t Image_Number, uint8_t *RAM_address, bool Debug)
{
    uint8_t *Flash_Absolute_Address;

    if (Debug)
        printf("Copy image %d to RAM\r\n", Image_Number);

    // Range check
    if (Image_Number >= IMAGE_NUMBER)
    {
        if (Debug)
            printf("Invalid image number %d (0-%d)\r\n", Image_Number, IMAGE_NUMBER);
        return (-1);
    }

    // Caculate absolute flash image add9ress
    Flash_Absolute_Address = (uint8_t *)(IMAGE_ABSOLUTE_ADDRESS + (Image_Number * IMAGE_SIZE));
    memcpy(RAM_address, Flash_Absolute_Address, IMAGE_SIZE);

    return (0);
}

// *****************************************************************************
//
// Erase the all Image Flash memory NOTE Takes A long time to run ( 1 MIN )
//
void Flash_Erase_All(void)
{
    uint32_t ints;

    // Need to stop core 1 from running while messing with FLASH
    if (Has_Core_One_Been_Stated())
        multicore_lockout_start_blocking();

    // Do erase with interrupt OFF
    ints = save_and_disable_interrupts();
    flash_range_erase(IMAGE_OFFSET_ADDRESS, IMAGE_SIZE * IMAGE_NUMBER);
    restore_interrupts(ints);

    // Restart core 1
    if (Has_Core_One_Been_Stated())
        multicore_lockout_end_blocking();
}

// *****************************************************************************
// Erase image from flash
int Flash_Erase_Image(uint8_t Image_Number, bool Debug)
{
    uint32_t ints;
    uint32_t Flash_Offset_Address;

    if (Debug)
        printf("Erasing image %d\r\n", Image_Number);

    // Range check
    if (Image_Number >= IMAGE_NUMBER)
    {
        if (Debug)
            printf("Invalid image number %d (0-%d)\r\n", Image_Number, IMAGE_NUMBER);
        return (-1);
    }
    // Caculate offset address in flash
    Flash_Offset_Address = IMAGE_OFFSET_ADDRESS + (Image_Number * IMAGE_SIZE);

    // Need to stop core 1 from running while messing with FLASH
    if (Has_Core_One_Been_Stated())
        multicore_lockout_start_blocking();

    // Do erase with interrupt OFF
    ints = save_and_disable_interrupts();
    flash_range_erase(Flash_Offset_Address, IMAGE_SIZE);
    restore_interrupts(ints);

    // Restart core 1
    if (Has_Core_One_Been_Stated())
        multicore_lockout_end_blocking();

    return (0);
}

// *****************************************************************************
// Store image from ram into flash
int Flash_Image_Store(uint8_t Image_Number, uint8_t *RAM_address, bool Debug)
{
    uint32_t ints;
    uint32_t Flash_Offset_Address;

    // Must erase before writing
    if (Flash_Erase_Image(Image_Number, Debug) < 0)
    {
        if (Debug)
            printf("Unable to store image %d\r\n", Image_Number);
        return (-1);
    }
    if (Debug)
        printf("Sotiring image in flash%d\r\n", Image_Number);

    // Caculate offset address in flash
    Flash_Offset_Address = IMAGE_OFFSET_ADDRESS + (Image_Number * IMAGE_SIZE);

    // Need to lock core 1 while messing with FLASH
    if (Has_Core_One_Been_Stated())
        multicore_lockout_start_blocking();

    // Writing to flash with interrupt OFF
    ints = save_and_disable_interrupts();
    flash_range_program(Flash_Offset_Address, RAM_address, IMAGE_SIZE);
    restore_interrupts(ints);

    // Unlock core 1
    if (Has_Core_One_Been_Stated())
        multicore_lockout_end_blocking();

    return (0);
}

// *****************************************************************************
// Return Absolute memory address for an image
uint8_t *Flash_Image_Address(uint8_t Image_Number, bool Debug)
{
    uint8_t *Flash_Absolute_Address;

    if (Debug)
        printf("Flash address for image %d = ", Image_Number);

    // Range check
    if (Image_Number >= IMAGE_NUMBER)
    {
        if (Debug)
            printf("Invalid image number %d (0-%d)\r\n", Image_Number, IMAGE_NUMBER);
        return (0);
    }

    Flash_Absolute_Address = (uint8_t *)(IMAGE_ABSOLUTE_ADDRESS + (Image_Number * IMAGE_SIZE));

    if (Debug)
        printf("%ld,%ld\r\n", Flash_Absolute_Address);

    return (Flash_Absolute_Address);
}
