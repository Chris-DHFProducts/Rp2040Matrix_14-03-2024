/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Sweep through all 7-bit I2C addresses, to see if any slaves are present on
// the I2C bus. Print out a table that looks like this:
//
// I2C Bus Scan
//   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// 0
// 1       @
// 2
// 3             @
// 4
// 5
// 6
// 7
//
// E.g. if slave addresses 0x12 and 0x34 were acknowledged.
//
// Perform a 1-byte dummy read from the probe address. If a slave
// acknowledges this address, the function returns the number of bytes
// transferred. If the address byte is ignored, the function returns
// -1.

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "i2cPico.h"

// ***************************************************************************
// Keep a list of active devices on the i2c bus
//
// Used to reduce the number of requests on the i2c bus
// when just checking
//
bool DeviceList[128];

bool DoneFirstScan = false;

// ***************************************************************************
//
// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

// ***************************************************************************
// Use to check if a device has been found on the i2c bus during last scan
bool i2c_device_scaned_ok(uint8_t deviceAddress)
{
    if (deviceAddress > 127 && !DoneFirstScan)
        return (false);
    else
        return (DeviceList[deviceAddress]);
}

// ***************************************************************************
//
void i2c_bus_setup_scan(uint8_t port, uint8_t sda_pin, uint8_t scl_pin, uint32_t CLK_Speed)
{
    i2c_inst_t *i2c_port;
    uint8_t rxdata;
    uint8_t addr;

    // Use the correct i2c port
    if (port)
        i2c_port = i2c1;
    else
        i2c_port = i2c0;

    I2C_Setup(port, sda_pin, scl_pin, CLK_Speed);

    printf("\n\rI2C Bus Scan\n\r");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n\r");

    // Scan addresses 127 to 0
    addr = 128;
    while (addr--)
    {
        if (addr % 16 == 0) // Debug output table formatting
        {
            printf("%02x ", addr);
        }

        DeviceList[addr] = false; // Default no connection on i2c bus

        if (reserved_addr(addr)) // Skip over any reserved addresses.
            printf("R");
        else
        {
            // Can we find the device of the i2c bus ?
            if (i2c_read_blocking(i2c_port, addr, &rxdata, 1, false) < 0)
            {
                printf("."); // if not found device
            }
            else
            {
                // if found device
                printf("@");
                DeviceList[addr] = true;
            }
        }

        // Debug output table formatting
        printf(addr % 16 == 15 ? "\n\r" : "  ");
    }

    DoneFirstScan = true;
    printf("Done.\n\r");
}
