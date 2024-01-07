/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "rng.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "HD44780.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define LINE_MAX_LENGTH 80
#define PREFIX_LENGTH 5
#define TIMEOUT_MS 50
#define BUFFER_SIZE 5

static char line_buffer_debug[LINE_MAX_LENGTH + 1];
static char line_buffer_debug_GSM[LINE_MAX_LENGTH + 1];
static char line_buffer_debug_BLE[LINE_MAX_LENGTH + 1];
static char line_buffer_bluetooth[LINE_MAX_LENGTH + 1];
static char line_buffer_gsm[LINE_MAX_LENGTH + 1];

static uint32_t line_lenght_debug;
static uint32_t line_lenght_debug_BLE;
static uint32_t line_lenght_debug_GSM;
static uint32_t line_lenght_bluetooth;
static uint32_t line_lenght_gsm;

bool if_send_end_line = false;
bool if_phone_number_set = false;
bool if_phone_number_set_latch = false;
bool can_send_sms = false;
bool if_door_open;
bool if_key_pressed = false;

uint32_t last_byte_time = 0;
uint32_t last_command_time = 0;

char phone_number[10];
char access_key[5];
char key_buffer[5];

void send_SMS(void);
void delay(uint32_t iterations);
void add_digit(char digit);

void line_append_debug(uint8_t value)
{
	if(value == '\r' || value == '\n')
	{
		if(line_lenght_debug >= 0)
		{
			if(strncmp(line_buffer_debug, "B ", 2) == 0)
			{
				strncpy(line_buffer_debug_BLE, &line_buffer_debug[2], (LINE_MAX_LENGTH - 2));
				line_lenght_debug_BLE = line_lenght_debug - 2;
				line_buffer_debug_BLE[line_lenght_debug_BLE++] = '\r';
				line_buffer_debug_BLE[line_lenght_debug_BLE++] = '\n';
				line_buffer_debug_BLE[line_lenght_debug_BLE] = '\0';
				HAL_UART_Transmit_IT(&huart3, (uint8_t*)line_buffer_debug_BLE, line_lenght_debug_BLE);
			} else if (strncmp(line_buffer_debug, "G ", 2) == 0)
			{
				strncpy(line_buffer_debug_GSM, &line_buffer_debug[2], (LINE_MAX_LENGTH - 2));
				line_lenght_debug_GSM = line_lenght_debug - 2;
				line_buffer_debug_GSM[line_lenght_debug_GSM++] = '\n';
				line_buffer_debug_GSM[line_lenght_debug_GSM] = '\0';
				HAL_UART_Transmit_IT(&huart1, (uint8_t*)line_buffer_debug_GSM, line_lenght_debug_GSM);
			}
			line_buffer_debug[line_lenght_debug] = '\0';
			if_send_end_line = true;
			HAL_UART_Transmit_IT(&huart2, (uint8_t*)line_buffer_debug, strlen(line_buffer_debug));
			line_lenght_debug = 0;
		}
	} else
	{
		if(line_lenght_debug >= LINE_MAX_LENGTH)
		{
			line_lenght_debug = 0;
		}
		line_buffer_debug[line_lenght_debug++] = value;
	}
}

void line_append_bluetooth(uint8_t value)
{

	if(value == '\r' || value == '\n')
	{
		if(line_lenght_bluetooth > 0)
		{
			line_buffer_bluetooth[line_lenght_bluetooth] = '\0';
			if_send_end_line = true;
			HAL_UART_Transmit_IT(&huart2, (uint8_t*)line_buffer_bluetooth, strlen(line_buffer_bluetooth));
			line_lenght_bluetooth = 0;
		}
	} else if(value == '#')
	{
		if(line_lenght_bluetooth > 0)
		{
			strncpy(phone_number, line_buffer_bluetooth, 9);
			phone_number[9] = '\0';
			if_send_end_line = true;
			if_phone_number_set = true;
			if_phone_number_set_latch = true;
			HAL_UART_Transmit_IT(&huart2, (uint8_t*)phone_number, strlen(phone_number));
			line_lenght_bluetooth = 0;

			send_SMS();
		}
	} else
	{
		if(line_lenght_bluetooth >= LINE_MAX_LENGTH)
		{
			line_lenght_bluetooth = 0;
		}
		line_buffer_bluetooth[line_lenght_bluetooth++] = value;
	}
}

void line_append_gsm(uint8_t value)
{
	if(line_lenght_gsm < LINE_MAX_LENGTH)
	{
		line_buffer_gsm[line_lenght_gsm++] = value;
		last_byte_time = HAL_GetTick();
	} else
	{
		line_lenght_gsm = 0;
	}
}

void check_timeout_gsm(void)
{
	if(line_lenght_gsm > 0 && (HAL_GetTick() - last_byte_time > TIMEOUT_MS))
	{
		line_buffer_gsm[line_lenght_gsm] = '\0';
//		if_send_end_line = true;
		HAL_UART_Transmit_IT(&huart2, (uint8_t*)line_buffer_gsm, strlen(line_buffer_gsm));
		line_lenght_gsm = 0;
	}
}

void send_end_line(void)
{
	static char end_signs[3] = "\r\n\0";

	if(if_send_end_line == true)
	{
		HAL_UART_Transmit_IT(&huart2, (uint8_t*)end_signs, 3);
		if_send_end_line = false;
	}
}

uint8_t uart2_rx_buffer, uart1_rx_buffer, uart3_rx_buffer;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart2)
	{
		line_append_debug(uart2_rx_buffer);
		HAL_UART_Receive_IT(&huart2, &uart2_rx_buffer, 1);
	}
	else if(huart == &huart1)
	{
		line_append_gsm(uart1_rx_buffer);
		HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer, 1);
	}
	else if(huart == &huart3)
	{
		line_append_bluetooth(uart3_rx_buffer);
		HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);
	}
}

void access_key_draw(void)
{
	static uint32_t random_number;
	static uint16_t access_code;

	if(if_phone_number_set == true)
	{
		HAL_RNG_GenerateRandomNumber(&hrng, &random_number);
		access_code = random_number % 10000;
		sprintf(access_key, "%04u", access_code);
		if_phone_number_set = false;
	}
}

typedef enum{
	MESSAGE_1,
	MESSAGE_2,
	MESSAGE_3,
	MESSAGE_4,
	MESSAGE_5,
	DONE
}sender_state;

int message_number = MESSAGE_1;

void send_SMS(void)
{
	static char message_cmgf[] = "AT+CMGF=1\r";
	static char message_cscs[] = "AT+CSCS=\"GSM\"\r";
	static char message_cmgs[31];
	sprintf(message_cmgs, "AT+CMGS=\"+48%s\"\r", phone_number);
	access_key_draw();
	static char message_message[34];
	sprintf(message_message, "Kod dostepu do skrytki: %s", access_key);
	static char message_ctrlz = 0x1A;

	delay(100);

	switch(message_number)
	{
	case MESSAGE_1:
		HAL_UART_Transmit_IT(&huart1, (uint8_t*)message_cmgf, strlen(message_cmgf));
		message_number = MESSAGE_2;
		break;
	case MESSAGE_2:
		HAL_UART_Transmit_IT(&huart1, (uint8_t*)message_cscs, strlen(message_cscs));
		message_number = MESSAGE_3;
		break;
	case MESSAGE_3:
		HAL_UART_Transmit_IT(&huart1, (uint8_t*)message_cmgs, strlen(message_cmgs));
		message_number = MESSAGE_4;
		break;
	case MESSAGE_4:
		HAL_UART_Transmit_IT(&huart1, (uint8_t*)message_message, strlen(message_message));
		message_number = MESSAGE_5;
		break;
	case MESSAGE_5:
		HAL_UART_Transmit_IT(&huart1, (uint8_t*)&message_ctrlz, 1);
		message_number = DONE;
		break;
	default:
		break;
	}
}

void send_AT_init(void)
{
	static char AT_init[] = "AT\r\n";

	HAL_UART_Transmit_IT(&huart1, (uint8_t*)AT_init, strlen(AT_init));
	HAL_UART_Transmit_IT(&huart2, (uint8_t*)AT_init, strlen(AT_init));
	HAL_UART_Transmit_IT(&huart3, (uint8_t*)AT_init, strlen(AT_init));
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart1)
	{
		if(if_phone_number_set_latch == true)
		{
			send_SMS();
		}
	}
	if(huart == &huart2)
	{
		send_end_line();
	}
}

GPIO_InitTypeDef GPIO_InitStructPrivate = {0};
uint32_t previousMillis = 0;
uint32_t currentMillis = 0;
volatile uint8_t pressed_key = '\0';

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	currentMillis = HAL_GetTick();
	if(currentMillis - previousMillis > 200 )
	{
		GPIO_InitStructPrivate.Pin = COL_1_Pin|COL_2_Pin|COL_3_Pin|COL_4_Pin;
		GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
		GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
		GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(COL_1_GPIO_Port, &GPIO_InitStructPrivate);

		HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, 1);
		HAL_GPIO_WritePin(ROW_2_GPIO_Port, ROW_2_Pin, 0);
		HAL_GPIO_WritePin(ROW_3_GPIO_Port, ROW_3_Pin, 0);
		HAL_GPIO_WritePin(ROW_4_GPIO_Port, ROW_4_Pin, 0);
		if(GPIO_Pin == COL_1_Pin && HAL_GPIO_ReadPin(COL_1_GPIO_Port, COL_1_Pin))
		{
			pressed_key = '1';
		} else if(GPIO_Pin == COL_2_Pin && HAL_GPIO_ReadPin(COL_2_GPIO_Port, COL_2_Pin))
		{
			pressed_key = '2';
		} else if(GPIO_Pin == COL_3_Pin && HAL_GPIO_ReadPin(COL_3_GPIO_Port, COL_3_Pin))
		{
			pressed_key = '3';
		} else if(GPIO_Pin == COL_4_Pin && HAL_GPIO_ReadPin(COL_4_GPIO_Port, COL_4_Pin))
		{
			pressed_key = 'A';
		}

		HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, 0);
		HAL_GPIO_WritePin(ROW_2_GPIO_Port, ROW_2_Pin, 1);
		HAL_GPIO_WritePin(ROW_3_GPIO_Port, ROW_3_Pin, 0);
		HAL_GPIO_WritePin(ROW_4_GPIO_Port, ROW_4_Pin, 0);
		if(GPIO_Pin == COL_1_Pin && HAL_GPIO_ReadPin(COL_1_GPIO_Port, COL_1_Pin))
		{
			pressed_key = '4';
		} else if(GPIO_Pin == COL_2_Pin && HAL_GPIO_ReadPin(COL_2_GPIO_Port, COL_2_Pin))
		{
			pressed_key = '5';
		} else if(GPIO_Pin == COL_3_Pin && HAL_GPIO_ReadPin(COL_3_GPIO_Port, COL_3_Pin))
		{
			pressed_key = '6';
		} else if(GPIO_Pin == COL_4_Pin && HAL_GPIO_ReadPin(COL_4_GPIO_Port, COL_4_Pin))
		{
			pressed_key = 'B';
		}

		HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, 0);
		HAL_GPIO_WritePin(ROW_2_GPIO_Port, ROW_2_Pin, 0);
		HAL_GPIO_WritePin(ROW_3_GPIO_Port, ROW_3_Pin, 1);
		HAL_GPIO_WritePin(ROW_4_GPIO_Port, ROW_4_Pin, 0);
		if(GPIO_Pin == COL_1_Pin && HAL_GPIO_ReadPin(COL_1_GPIO_Port, COL_1_Pin))
		{
			pressed_key = '7';
		} else if(GPIO_Pin == COL_2_Pin && HAL_GPIO_ReadPin(COL_2_GPIO_Port, COL_2_Pin))
		{
			pressed_key = '8';
		} else if(GPIO_Pin == COL_3_Pin && HAL_GPIO_ReadPin(COL_3_GPIO_Port, COL_3_Pin))
		{
			pressed_key = '9';
		} else if(GPIO_Pin == COL_4_Pin && HAL_GPIO_ReadPin(COL_4_GPIO_Port, COL_4_Pin))
		{
			pressed_key = 'C';
		}

		HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, 0);
		HAL_GPIO_WritePin(ROW_2_GPIO_Port, ROW_2_Pin, 0);
		HAL_GPIO_WritePin(ROW_3_GPIO_Port, ROW_3_Pin, 0);
		HAL_GPIO_WritePin(ROW_4_GPIO_Port, ROW_4_Pin, 1);
		if(GPIO_Pin == COL_1_Pin && HAL_GPIO_ReadPin(COL_1_GPIO_Port, COL_1_Pin))
		{
			pressed_key = '*';
		} else if(GPIO_Pin == COL_2_Pin && HAL_GPIO_ReadPin(COL_2_GPIO_Port, COL_2_Pin))
		{
			pressed_key = '0';
		} else if(GPIO_Pin == COL_3_Pin && HAL_GPIO_ReadPin(COL_3_GPIO_Port, COL_3_Pin))
		{
			pressed_key = '#';
		} else if(GPIO_Pin == COL_4_Pin && HAL_GPIO_ReadPin(COL_4_GPIO_Port, COL_4_Pin))
		{
			pressed_key = 'D';
		}

		  HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, 1);
		  HAL_GPIO_WritePin(ROW_2_GPIO_Port, ROW_2_Pin, 1);
		  HAL_GPIO_WritePin(ROW_3_GPIO_Port, ROW_3_Pin, 1);
		  HAL_GPIO_WritePin(ROW_4_GPIO_Port, ROW_4_Pin, 1);
		  GPIO_InitStructPrivate.Mode = GPIO_MODE_IT_RISING;
		  GPIO_InitStructPrivate.Pull = GPIO_PULLDOWN;
		  HAL_GPIO_Init(COL_1_GPIO_Port, &GPIO_InitStructPrivate);

		  if((pressed_key == '1'|| pressed_key == '2'|| pressed_key == '3'|| pressed_key == '4'|| pressed_key == '5'|| pressed_key == '6'|| pressed_key == '7'|| pressed_key == '8'|| pressed_key == '9'|| pressed_key == '0') && if_phone_number_set_latch == true)
		  		  {
		  			  char pressed_digit = pressed_key;
		  			  add_digit(pressed_digit);
		  			  pressed_key = '\0';
		  			  if_key_pressed = true;
		  		  }

		  previousMillis = currentMillis;

	}
}

int current_index = 0;

void add_digit(char digit)
{
	if(current_index < BUFFER_SIZE - 1)
	{
		key_buffer[current_index++] = digit;
		key_buffer[current_index] = '\0';
	}
}

void reset_buffer()
{
	memset(key_buffer, 0, BUFFER_SIZE);
	current_index = 0;
}

void delay(uint32_t iterations)
{
	while(iterations-- > 0)
	{
		__NOP();
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RNG_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  lcd_init();
  send_AT_init();

  lcd_backlight(1);
  lcd_clear();

  memset(key_buffer, '\0', BUFFER_SIZE);

  init_lcd_check_door();

  HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, 1);
  HAL_GPIO_WritePin(ROW_2_GPIO_Port, ROW_2_Pin, 1);
  HAL_GPIO_WritePin(ROW_3_GPIO_Port, ROW_3_Pin, 1);
  HAL_GPIO_WritePin(ROW_4_GPIO_Port, ROW_4_Pin, 1);

  HAL_UART_Receive_IT(&huart2, &uart2_rx_buffer, 1);
  HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer, 1);
  HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);

  while (1)
  {
	  check_timeout_gsm();
	  lcd_check_door();
	  lcd_check_telephone(if_phone_number_set_latch);
	  if_key_pressed = lcd_display_key(key_buffer, if_key_pressed);
	  lcd_check_key(key_buffer, access_key);
	  if(HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_RESET)
	  {
		  reset_buffer();
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
