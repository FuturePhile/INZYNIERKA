/*
 * HD44780.c
 *
 *  Created on: Dec 29, 2023
 *      Author: FuturePhile
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "HD44780.h"
#include "i2c.h"

#define I2C_ADDR 0x27

#define RS_BIT 0
#define EN_BIT 2
#define BL_BIT 3
#define D4_BIT 4
#define D5_BIT 5
#define D6_BIT 6
#define D7_BIT 7

#define LCD_ROWS 2
#define LCD_COLS 16

#define TIMEOUT 100

static uint8_t backlight_state = 1;

static GPIO_PinState previous_state = GPIO_PIN_RESET;
static bool previous_state_telephone = true;
//static bool previous_state_key = false;

static void lcd_write_nibble(uint8_t nibble, uint8_t rs)
{
	uint8_t data = nibble << D4_BIT;
	data |= rs << RS_BIT;
	data |= backlight_state << BL_BIT;
	data |= 1 << EN_BIT;
	HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR << 1, &data, 1, TIMEOUT);
	HAL_Delay(1);
	data &= ~(1 << EN_BIT);
	HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR << 1, &data, 1, TIMEOUT);
}

void lcd_send_cmd(uint8_t cmd)
{
	uint8_t upper_nibble = cmd >> 4;
	uint8_t lower_nibble = cmd & 0x0F;
	lcd_write_nibble(upper_nibble, 0);
	lcd_write_nibble(lower_nibble, 0);
	if(cmd == 0x01 || cmd == 0x02)
	{
		HAL_Delay(2);
	}
}

void lcd_send_data(uint8_t data)
{
	uint8_t upper_nibble = data >> 4;
	uint8_t lower_nibble = data & 0x0F;
	lcd_write_nibble(upper_nibble, 1);
	lcd_write_nibble(lower_nibble, 1);
}

void lcd_init()
{
	HAL_Delay(50);
	lcd_write_nibble(0x03, 0);
	HAL_Delay(5);
	lcd_write_nibble(0x03, 0);
	HAL_Delay(1);
	lcd_write_nibble(0x03, 0);
	HAL_Delay(1);
	lcd_write_nibble(0x02, 0);
	lcd_send_cmd(0x28);
	lcd_send_cmd(0x0C);
	lcd_send_cmd(0x06);
	lcd_send_cmd(0x01);
	HAL_Delay(2);
}

void lcd_write_string(char *str)
{
	while(*str)
	{
		lcd_send_data(*str++);
	}
}

void lcd_set_cursor(uint8_t row, uint8_t column)
{
	uint8_t address;
	switch (row)
	{
		case 0:
			address = 0x00;
			break;
		case 1:
			address = 0x40;
			break;
		default:
			address = 0x00;
	}
	address += column;
	lcd_send_cmd(0x80 | address);
}

void lcd_clear(void)
{
	lcd_send_cmd(0x01);
	HAL_Delay(2);
}

void lcd_backlight(uint8_t state)
{
	if(state)
	{
		backlight_state = 1;
	} else
	{
		backlight_state = 0;
	}
}

void init_lcd_check_door(void)
{
	GPIO_PinState current_state = HAL_GPIO_ReadPin(MAG_SWITCH_GPIO_Port, MAG_SWITCH_Pin);
	if(current_state == GPIO_PIN_SET)
	{
		previous_state = GPIO_PIN_RESET;
	} else
	{
		previous_state = GPIO_PIN_SET;
	}
}

void lcd_check_door(void)
{
	static char *door_open = "Dzwi otwarte";
	static char *door_close = "Dzwi zamkniete";
	static char *clear = "                ";

	GPIO_PinState current_state = HAL_GPIO_ReadPin(MAG_SWITCH_GPIO_Port, MAG_SWITCH_Pin);

	if(current_state != previous_state)
	{
		if(current_state == GPIO_PIN_RESET)
		{
			lcd_set_cursor(0, 0);
			lcd_write_string(clear);
			lcd_set_cursor(0, 0);
			lcd_write_string(door_close);
		} else
		{
			lcd_set_cursor(0, 0);
			lcd_write_string(clear);
			lcd_set_cursor(0, 0);
			lcd_write_string(door_open);
		}
		previous_state = current_state;
	}
}

void lcd_check_telephone(bool current_state_telephone)
{
	static char *telephone_not_set = "Wyslij nr. tel.";
	static char *telephone_set = "Wpisz kod: ";
	static char *clear = "                ";

	if(current_state_telephone != previous_state_telephone)
	{
		if(current_state_telephone == false)
		{
			lcd_set_cursor(1, 0);
			lcd_write_string(clear);
			lcd_set_cursor(1, 0);
			lcd_write_string(telephone_not_set);
		} else
		{
			lcd_set_cursor(1, 0);
			lcd_write_string(clear);
			lcd_set_cursor(1, 0);
			lcd_write_string(telephone_set);
		}
		previous_state_telephone = current_state_telephone;
	}
}
bool lcd_display_key(char *key_buffer, bool current_state_key)
{
	static char *clear = "     ";

	if(current_state_key == true)
	{
		lcd_set_cursor(1,11);
		lcd_write_string(clear);
		lcd_set_cursor(1, 11);
		lcd_write_string(key_buffer);
		current_state_key = false;
	}
	return current_state_key;
}

void lcd_check_key(char *key_buffer, char *access_key)
{
	static char *clear = "                ";
	static char *good = "Klucz poprawny";
	static char *bad = "Zly klucz";

	if(strlen(key_buffer) == 4)
	{
		if(strcmp(key_buffer, access_key) == 0)
			{
				lcd_set_cursor(1, 0);
				lcd_write_string(clear);
				lcd_set_cursor(1, 0);
				lcd_write_string(good);
			} else
			{
				lcd_set_cursor(1, 0);
				lcd_write_string(clear);
				lcd_set_cursor(1, 0);
				lcd_write_string(bad);
			}
	}
}
