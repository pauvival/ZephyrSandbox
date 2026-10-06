//GUIWAY

//hap_driver
//Driver for the haptic actuators (at this stage of development they are LEDS) 

#ifndef _LED_DRIVER_H_
#define _LED_DRIVER_H_

uint32_t leds_init(void);
uint32_t led_set_state(uint32_t index);
void leds_set_all(void);
uint32_t led_reset_state(uint32_t index);
void leds_reset_all(void);
uint32_t led_toggle(uint32_t index);
void leds_toggle_all(void);

#endif