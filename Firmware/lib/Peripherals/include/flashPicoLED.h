#ifndef __FLASH_PICO_LED_H_
#define __FLASH_PICO_LED_H_

#define RP2040_HW_LED_DEF_PIN 25

#define RP2040_HW_LED_OFF          0
#define RP2040_HW_LED_FLASHING_ON  1
#define RP2040_HW_LED_FLASHING_OFF 2

void flashPicoLED_Init(uint8_t pin, uint8_t state, uint32_t interval);
void flashPicoLED();

#endif // __FLASH_PICO_LED_H_