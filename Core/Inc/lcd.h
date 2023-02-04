#ifndef _LCD_H_
#define _LCD_H_

#include "stm32h7xx_hal.h"
#include <stdint.h>

void lcd_init(SPI_HandleTypeDef *spi);
void lcd_backlight_on();
void lcd_backlight_off();
#endif
