
// ***************************************************************************
//
#include "stdio.h"
#include "pico/stdlib.h"

#include "inputs.h"

bool LED_State = false;

uint32_t LED_Toggle_Delay_Counter;
uint32_t LED_Toggle_Delay_Max;

// ***************************************************************************
//
void LED_On()
{
	// Setup test switch input (Inverted)
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	gpio_put(LED_PIN, 1);
	LED_State = true;
}

// ***************************************************************************
//
void LED_Toggle(int32_t Current, int32_t Wait)
{
	if (Current > 0)
	{
		if (Current > Wait)
			LED_Toggle_Delay_Counter = 0x00;
		else
			LED_Toggle_Delay_Counter = Current;
	}

	if (Wait > 0)
	{
		LED_Toggle_Delay_Max = Current;
		LED_Toggle_Delay_Counter = 0x00;
	}
	
	if (LED_Toggle_Delay_Counter == LED_Toggle_Delay_Max)
	{
		LED_Toggle_Delay_Counter = 0;
		gpio_init(LED_PIN);
		gpio_set_dir(LED_PIN, GPIO_OUT);

		if (LED_State)
		{
			LED_State = false;
			gpio_put(LED_PIN, 0);
		}
		else
		{
			LED_State = true;
			gpio_put(LED_PIN, 1);
		}
	}
	else
		LED_Toggle_Delay_Counter++;
}

// ***************************************************************************
//
void LED_Pluse(uint32_t delay, uint number)
{

	// Setup test switch input (Inverted)
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	while (number--)
	{
		gpio_put(LED_PIN, 1);
		sleep_ms(delay);

		gpio_put(LED_PIN, 0);
		sleep_ms(delay);
	}
	LED_State = false;
}

// ***************************************************************************
// The Switch input is inverted
bool Input_Test_Switch()
{
	return (!gpio_get(INPUT_TEST_SWITCH_GPIO));
}

// ***************************************************************************
//
void Input_Setup(void)
{

	// Setup test switch input (Inverted)
	gpio_init(INPUT_TEST_SWITCH_GPIO);
	gpio_set_dir(INPUT_TEST_SWITCH_GPIO, GPIO_IN);
	gpio_pull_up(INPUT_TEST_SWITCH_GPIO);

	// Setup Switch inputs
	gpio_init(INPUT_SWITCH_BASE + 0);
	gpio_init(INPUT_SWITCH_BASE + 1);
	gpio_init(INPUT_SWITCH_BASE + 2);
	gpio_init(INPUT_SWITCH_BASE + 3);

	gpio_set_dir(INPUT_SWITCH_BASE + 0, GPIO_IN);
	gpio_set_dir(INPUT_SWITCH_BASE + 1, GPIO_IN);
	gpio_set_dir(INPUT_SWITCH_BASE + 2, GPIO_IN);
	gpio_set_dir(INPUT_SWITCH_BASE + 3, GPIO_IN);

	gpio_pull_up(INPUT_SWITCH_BASE + 0);
	gpio_pull_up(INPUT_SWITCH_BASE + 1);
	gpio_pull_up(INPUT_SWITCH_BASE + 2);
	gpio_pull_up(INPUT_SWITCH_BASE + 3);

	// Setup Opto Inputs
	gpio_init(INPUT_OPTO_BASE + 0);
	gpio_init(INPUT_OPTO_BASE + 1);
	gpio_init(INPUT_OPTO_BASE + 2);
	gpio_init(INPUT_OPTO_BASE + 3);

	gpio_set_dir(INPUT_OPTO_BASE + 0, GPIO_IN);
	gpio_set_dir(INPUT_OPTO_BASE + 1, GPIO_IN);
	gpio_set_dir(INPUT_OPTO_BASE + 2, GPIO_IN);
	gpio_set_dir(INPUT_OPTO_BASE + 3, GPIO_IN);
}

// ***************************************************************************
//
uint8_t Input_Switch()
{
	int32_t Input = ~gpio_get_all();
	uint8_t Result;

	Input >>= INPUT_SWITCH_BASE; // Move all bits to right

	// Re-mapping switch inputs
	if(Input & 8)
		Result |= 1;
	if(Input & 4)
		Result |= 2;
	if(Input & 2)
		Result |= 4;
	if(Input & 1)
		Result |= 8;

	// Mask off unused switched
	Result &= INPUT_SWITCH_MASK;
	return (Result);
}

uint8_t Input_Opto()
{
	int32_t Result = ~gpio_get_all();

	Result >>= INPUT_OPTO_BASE; // Move all bits to right
	Result &= INPUT_OPTO_MASK;	// Clear out any remaining
	return (Result);
}