#ifndef BSP_GPIO_H
#define BSP_GPIO_H
#include <stdbool.h>
void bsp_gpio_init(void);
void bsp_led_set(bool on);
void bsp_led_toggle(void);
bool bsp_button_read(void);
#endif
