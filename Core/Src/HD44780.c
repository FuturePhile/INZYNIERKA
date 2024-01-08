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
#include "main.h"

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

#define delay_second 16000000

static uint8_t backlight_state = 1;

static bool previous_state_telephone = true;
static bool can_enter_key = false;

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

typedef enum{
	LCD_1,
	LCD_2,
	LCD_3,
	LCD_4,
	LCD_5,
	LCD_6,
	LCD_7,
	LCD_8,
	RST,
	DONE
}lcd_state;

int lcd_number = LCD_1;

void lcd_display(bool current_state_telephone, bool current_state_key, char *key_buffer, char *access_key, char pressed_button, char *ble_cmd)
{
	static char *door_open = "Dzwi otwarte";
	static char *door_closed = "Dzwi zamkniete";
	static char *telephone_not_set = "Wprowadz nr. tel.";
	static char *telephone_set = "Wpisz kod: ";
	static char *key_good = "Klucz poprawny";
	static char *key_bad = "Zly klucz";
	static char *message_1 = "Nacisnij A po  ";
	static char *message_2 = "odebraniu paczki";
	static char *unlock = "Odblokuj";
	static char *lock = "Zablokuj";
	static char *reset = "Zresetuj";
	static char *clear_line = "                ";
//	static char *clear_key = "    ";

	static bool if_key_correct = false;
	static bool if_key_entered = false;
	static bool cmd_1 = false;
	static bool cmd_2 = false;
	static bool cmd_3 = false;


	if(current_state_telephone != previous_state_telephone)
	{
		if(current_state_telephone == false)
		{
			lcd_number = LCD_1;
		}else if(current_state_telephone == true)
		{
			lcd_number = LCD_2;
		}
		previous_state_telephone = current_state_telephone;
	}

	if(strlen(key_buffer) == 4)
	{
		if_key_entered = true;
		if(strcmp(key_buffer, access_key) == 0)
		{
			if_key_correct = true;
		}
	}

	if(if_key_entered == true)
	{
		if(if_key_correct == true)
		{
			lcd_number = LCD_4;
		} else
		{
			lcd_number = LCD_3;
		}
		if_key_entered = false;
	}

	if(pressed_button == 'A' && if_key_correct == true)
	{
		lcd_number = LCD_6;
		if_key_correct = false;
	}

	if(strcmp(ble_cmd, "open") == 0)
	{
		if(cmd_1 == true)
		{
			lcd_number = LCD_7;
			cmd_1 = false;
		}
	}
	if(strcmp(ble_cmd, "close") == 0)
	{
		if(cmd_2 == true)
		{
			lcd_number = LCD_8;
			cmd_2 = false;
		}
	}
	if(strcmp(ble_cmd, "reset") == 0)
	{
		if(cmd_3 == true)
		{
			lcd_number = RST;
			cmd_3 = false;
		}
	}


	switch(lcd_number)
	{
	case LCD_1:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(door_open);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(telephone_not_set);

		lcd_number = DONE;
		break;
	case LCD_2:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(door_closed);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(telephone_set);

		can_enter_key = true;

		lcd_number = DONE;
		break;
	case LCD_3:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(door_closed);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(key_bad);

		can_enter_key = false;

		reset_buffer();

		delay(delay_second*2);

		lcd_number = LCD_2;
		break;
	case LCD_4:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(door_open);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(key_good);

		can_enter_key = false;

		reset_buffer();

		delay(delay_second*2);

		lcd_number = LCD_5;
		break;
	case LCD_5:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(message_1);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(message_2);

		lcd_number = DONE;
		break;
	case LCD_6:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(door_closed);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(unlock);

		cmd_1 = true;

		lcd_number = DONE;
		break;
	case LCD_7:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(door_open);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(lock);

		cmd_2 = true;

		lcd_number = DONE;
		break;
	case LCD_8:
		lcd_set_cursor(0, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(0, 0);
		lcd_write_string(door_closed);

		lcd_set_cursor(1, 0);
		lcd_write_string(clear_line);
		lcd_set_cursor(1, 0);
		lcd_write_string(reset);

		cmd_3 = true;

		lcd_number = DONE;
		break;
	case RST:
		NVIC_SystemReset();
		break;
	default:
		break;
	}

}

bool lcd_display_key(char *key_buffer, bool current_state_key)
{
	static char *clear = "     ";

	if(current_state_key == true && can_enter_key == true)
	{
		lcd_set_cursor(1,11);
		lcd_write_string(clear);
		lcd_set_cursor(1, 11);
		lcd_write_string(key_buffer);
		current_state_key = false;
	}
	return current_state_key;
}
