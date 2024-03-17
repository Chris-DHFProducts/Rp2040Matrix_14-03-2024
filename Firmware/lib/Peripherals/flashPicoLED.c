// ***************************************************************************
// System headers
//#include <stdio.h>
//#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ***************************************************************************
// Local headers
#include "flashPicoLED.h"

// ***************************************************************************
// Global variables
uint8_t Pico_LED_State;
uint8_t Pico_LED_PIN = RP2040_HW_LED_DEF_PIN;
uint32_t Pico_LED_Counter;
uint32_t Pico_LED_Interval;

// ***************************************************************************
//  pin      = hardware pin number
// 
//  interval = int32  "how many times flashPicoLED needs to be called to toggle LES state"
//
//  state    = RP2040_HW_LED_OFF           "No flashing"
//           = RP2040_HW_LED_FLASHING_OFF  "Flashing stating with OFF"
//           = RP2040_HW_LED_FLASHING_ON   "Flashing stating with ON"
//
void flashPicoLED_Init(uint8_t pin, uint8_t state, uint32_t interval) 
{
    Pico_LED_PIN      = pin;
    Pico_LED_State    = state;
    Pico_LED_Counter  = 0;
    Pico_LED_Interval = interval;
    
    // Configure hardware
    gpio_init(Pico_LED_PIN);
    gpio_set_dir(Pico_LED_PIN, GPIO_OUT);
    
    // Set the state of LED now
    switch(Pico_LED_State)
    {
        case RP2040_HW_LED_OFF:
            gpio_put(Pico_LED_PIN, 0);
            break;
        case RP2040_HW_LED_FLASHING_OFF:
            gpio_put(Pico_LED_PIN, 0);
            break;
        case RP2040_HW_LED_FLASHING_ON:
            gpio_put(Pico_LED_PIN, 1);
            break;
        default:
            Pico_LED_State = RP2040_HW_LED_OFF;
            gpio_put(Pico_LED_PIN, 0);
    }
}

// ***************************************************************************
// 
// Toggle the LED every 'interval' times this function is called
//
void flashPicoLED()
{
    Pico_LED_Counter++;
    if(Pico_LED_Counter >= Pico_LED_Interval)
    {
        Pico_LED_Counter = 0; 
        switch(Pico_LED_State)
        {
            case RP2040_HW_LED_OFF:
                gpio_put(Pico_LED_PIN, 0);
                break;
            case RP2040_HW_LED_FLASHING_OFF:
                gpio_put(Pico_LED_PIN, 1);
                Pico_LED_State = RP2040_HW_LED_FLASHING_ON;
                break;
            case RP2040_HW_LED_FLASHING_ON:
                gpio_put(Pico_LED_PIN, 0);
                Pico_LED_State = RP2040_HW_LED_FLASHING_OFF;
                break;
            default: // Error should not get here
                Pico_LED_State = RP2040_HW_LED_OFF;
                gpio_put(Pico_LED_PIN, 0);
        }            
    }
}