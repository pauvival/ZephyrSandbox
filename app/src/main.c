//ZEPHYR SANDBOX

//main.c 

//Generic libraries
#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

//User-made libraries
#include "bluetooth_mg.h"
#include "led_driver.h"
//1000 msec = 1 sec
#define SLEEP_TIME_MS   25


#define STACKSIZE 1024
#define THREAD0_PRIORITY 7
#define THREAD1_PRIORITY 7

LOG_MODULE_REGISTER(app,LOG_LEVEL_DBG);


void offload_led_control(struct k_work *work_term)
{

	LOG_INF("I am offload_led_control()\n");
	if(gatt_led_value == 0)
	{
		leds_reset_all();
	}
	else
	{
		leds_set_all();
	}
}

K_WORK_DEFINE(my_work, offload_led_control);

int main(void)
{
	LOG_INF("Hello World! %s\n", CONFIG_BOARD_TARGET);

	leds_init();
	init_BLE();

	
	while (1) 
	{
		
		k_work_submit(&my_work);
		LOG_INF("LED CONTROL THREAD!\n");
		LOG_INF("Now I have nothing to do :(\n");

		k_msleep(1000);
	}
	
	return 0;
}

int thread0(void){

	while(1)
	{
		LOG_INF("I am thread 0!\n");
		k_msleep(1000);
	}
	
	return 0;

}

int thread1(void){

	while(1)
	{
		LOG_INF("I am thread 1!\n");
		k_msleep(1000);
	}

	return 0;
	
}

K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL,
 THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL,
 THREAD1_PRIORITY, 0, 0);