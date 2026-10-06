//ZEPHYR SANDBOX

//main.c 

//Generic libraries
#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>

//User-made libraries
#include "bluetooth_mg.h"
#include "led_driver.h"
//1000 msec = 1 sec
#define SLEEP_TIME_MS   25


int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	leds_init();
	init_BLE();
	
	while (1) 
	{
		if(gatt_led_value == 0)
		{
			leds_reset_all();
		}
		else
		{
			leds_set_all();
		}

		k_msleep(500);
	}
	
	return 0;
}