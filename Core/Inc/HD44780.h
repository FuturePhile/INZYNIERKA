/*
 * HD44780.h
 *
 *  Created on: Dec 29, 2023
 *      Author: FuturePhile
 */
#pragma once
#ifndef INC_HD44780_H_
#define INC_HD44780_H_
#endif /* INC_HD44780_H_ */

#include "stm32l4xx.h"
#include <stdio.h>
#include <stdbool.h>

void lcd_init(void);
void lcd_send_cmd(uint8_t cmd);
void lcd_send_data(uint8_t data);
void lcd_write_string(char *str);
void lcd_set_cursor(uint8_t row, uint8_t column);
void lcd_clear(void);
void lcd_backlight(uint8_t state);
void lcd_display(bool current_state_telephone, bool current_state_key, char *key_buffer, char *access_key, char pressed_button, char *ble_cmd);
bool lcd_display_key(char *key_buffer, bool current_state_key);
