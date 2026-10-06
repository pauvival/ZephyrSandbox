#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "led_driver.h"

//Identify the parent devicetree node
#define LED_NODE DT_PATH(leds)

//Helper macro to initialize each element of the gpio_dt_spec array
#define LED_SPEC_INIT(node_id) GPIO_DT_SPEC_GET(node_id, gpios),

//Build an array of gpio_dt_spec populated from DTS child nodes
static const struct gpio_dt_spec leds[] = {
    DT_FOREACH_CHILD_STATUS_OKAY(LED_NODE, LED_SPEC_INIT)
};

//Obtain the number of LEDS, I do this in order to have flexibility in the future and experiment with dts macros
#define NUM_LEDS DT_CHILD_NUM_STATUS_OKAY(LED_NODE)

/**
 * @brief Initialize all configured LEDs.
 */
uint32_t leds_init(void)
{
    if (NUM_LEDS == 0) {
        printk("No LEDs configured in Devicetree.");
        return -ENODEV;
    }

    for (uint32_t i = 0; i < NUM_LEDS; i++) {
        if (!gpio_is_ready_dt(&leds[i])) {
            printk("GPIO device %s not ready for LED index %u", 
                    leds[i].port->name, i);
            return -ENODEV;
        }

        uint32_t ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            printk("Failed to configure GPIO for LED index %u (err: %d)", i, ret);
            return ret;
        }
    }

    printk("Successfully initialized %u LEDs", NUM_LEDS);
    return 0;
}

/**
 * @brief Set a specific LED by array index.
 */
uint32_t led_set_state(uint32_t index)
{
     if (index >= NUM_LEDS) {
        printk("Index %u out of bounds (max: %u)", index, NUM_LEDS - 1);
        return -EINVAL;
    }

    return gpio_pin_set_dt(&leds[index], true);
}

/**
 * @brief Set all LEDs simultaneously.
 */
void leds_set_all(void)
{
    for (size_t i = 0; i < NUM_LEDS; i++) {
        gpio_pin_set_dt(&leds[i], true);
    }
}

/**
 * @brief Reset a specific LED by array index.
 */
uint32_t led_reset_state(uint32_t index)
{
     if (index >= NUM_LEDS) {
        printk("Index %u out of bounds (max: %u)", index, NUM_LEDS - 1);
        return -EINVAL;
    }

    return gpio_pin_set_dt(&leds[index], false);
}

/**
 * @brief Reset all LEDs simultaneously.
 */
void leds_reset_all(void)
{
    for (size_t i = 0; i < NUM_LEDS; i++) {
        gpio_pin_set_dt(&leds[i], false);
    }
}

/**
 * @brief Toggle a specific LED by array index.
 */
uint32_t led_toggle(uint32_t index)
{
    if (index >= NUM_LEDS) {
        printk("Index %u out of bounds (max: %u)", index, NUM_LEDS - 1);
        return -EINVAL;
    }

    return gpio_pin_toggle_dt(&leds[index]);
}

/**
 * @brief Toggle all LEDs simultaneously.
 */
void leds_toggle_all(void)
{
    for (size_t i = 0; i < NUM_LEDS; i++) {
        gpio_pin_toggle_dt(&leds[i]);
    }
}