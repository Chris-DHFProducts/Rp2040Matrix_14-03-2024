#ifndef __INPUTS_H_
#define __INPUTS_H_

// ***************************************************************************
// LED output
#define LED_PIN 25

// ***************************************************************************
// GPIO inputs
#define INPUT_SWITCH_NUMBER 4 // How many switched are connected
#define INPUT_SWITCH_BASE 20  // Start GPIO start pin
#define INPUT_SWITCH_MASK 0b1111

#define INPUT_OPTO_NUMBER 4 // How many opto inputs are connected
#define INPUT_OPTO_BASE 26  // Start GPIO pin
#define INPUT_OPTO_MASK 0b1111

#define INPUT_TEST_SWITCH_GPIO 24

#define INPUT_SWITCH_1 0x01
#define INPUT_SWITCH_2 0x02
#define INPUT_SWITCH_3 0x04
#define INPUT_SWITCH_4 0x08

#define INPUT_SWITCH_00 0x00
#define INPUT_SWITCH_01 0x01
#define INPUT_SWITCH_02 0x02
#define INPUT_SWITCH_03 0x03
#define INPUT_SWITCH_04 0x04
#define INPUT_SWITCH_05 0x05
#define INPUT_SWITCH_06 0x06
#define INPUT_SWITCH_07 0x07
#define INPUT_SWITCH_08 0x08
#define INPUT_SWITCH_09 0x09
#define INPUT_SWITCH_10 0x0A
#define INPUT_SWITCH_11 0x0B
#define INPUT_SWITCH_12 0x0C
#define INPUT_SWITCH_13 0x0D
#define INPUT_SWITCH_14 0x0E
#define INPUT_SWITCH_15 0x0F

#define INPUT_OPTO_1 0x01
#define INPUT_OPTO_2 0x02
#define INPUT_OPTO_3 0x04
#define INPUT_OPTO_4 0x08

#define INPUT_OPTO_00 0x00
#define INPUT_OPTO_01 0x01
#define INPUT_OPTO_02 0x02
#define INPUT_OPTO_03 0x03
#define INPUT_OPTO_04 0x04
#define INPUT_OPTO_05 0x05
#define INPUT_OPTO_06 0x06
#define INPUT_OPTO_07 0x07
#define INPUT_OPTO_08 0x08
#define INPUT_OPTO_09 0x09
#define INPUT_OPTO_10 0x0A
#define INPUT_OPTO_11 0x0B
#define INPUT_OPTO_12 0x0C
#define INPUT_OPTO_13 0x0D
#define INPUT_OPTO_14 0x0E
#define INPUT_OPTO_15 0x0F

void LED_On();
void LED_Toggle(int32_t Current, int32_t Wait);
void LED_Pluse(uint32_t delay, uint number);

void Input_Setup(void);
bool Input_Test_Switch();
uint8_t Input_Switch();
uint8_t Input_Opto();

#endif // __INPUTS_H_