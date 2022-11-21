/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "rng.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"
#include "ILI9341_Touchscreen.h"

#include "Storage.h"
#include "MusicPlayer.h"

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

static char g_strBuf[256];

static const int UART_PrintF(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(g_strBuf, sizeof(g_strBuf), fmt, args);
	va_end(args);

	HAL_UART_Transmit(&huart3, (uint8_t*)g_strBuf, len, -1);
	return len;
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint32_t g_cooldownRecord = 1000;
static uint32_t g_cooldownLCD = 1000;

//LCD

#define MAX_SONG_LEN_SEC	(uint32_t)(10 * 60)		//10 min = 12 ksamples = 12kiB

int bCountEnable = false;
uint32_t count_1;
uint32_t sec;
uint32_t min;

void DisplayTimeEx(uint32_t sec, uint16_t x, uint16_t y) {
	if (sec < MAX_SONG_LEN_SEC) {
		sprintf(g_strBuf, "%01d.%02d", sec / 60, sec % 60);
	}
	else {
		strcpy(g_strBuf, "9.59");
	}
	ILI9341_Draw_Text(g_strBuf, x, y, WHITE,2, BLACK);
}
void SetScreen(void){
	 //Screen Setting
	 ILI9341_Set_Rotation(SCREEN_HORIZONTAL_1);
	 ILI9341_Fill_Screen(BLACK);

	//TOPIC
	 ILI9341_Draw_Text("D",90 ,15, RED,2, BLACK);
	 ILI9341_Draw_Text("N",105 ,15, YELLOW,2, BLACK);
	 ILI9341_Draw_Text("K",120 ,15, BLUE,2, BLACK);
	 ILI9341_Draw_Text("MUSIC BOX",140 ,15, WHITE,2, BLACK);

	 g_cooldownLCD = 500;
}

typedef struct {
	int Length; //(sec)
	bool empty;
} SlotMusic;

void SlotMusic_New(SlotMusic* slot, int length) {	//FOR DEBUGGING ONLY
	slot->Length = length;
	slot->empty = false;
}
void SlotMusic_NewEmpty(SlotMusic* slot) {
	slot->Length = 0;
	slot->empty = true;
}

static SlotMusic s[10];
static int CalculateLength(uint32_t size) {
	return (size - 4) / TIMERFREQ_LOADKEYS;
}
void SlotMusic_LoadFromStorage() {
	for (int i = 0; i < 10; ++i) {
		uint32_t addr = MusicPlayer_GetSongAddr(i);

		uint32_t size;
		StorageFlash_Read(addr, 4, &size);

		if (size == 0 || size == 0xffffffff) {
			s[i].Length = 0;
		}
		else {
			//(TotalSize - HeaderSize) / BytePerSecond
			s[i].Length = CalculateLength(size);
		}
		s[i].empty = s[i].Length == 0;
	}
}

//Music

static MusicPlayer musicPlayer;

static int ticks[4] = { 0, 0, 0, 0 };

void _My_TIM1_IRQHandler() {
	if (musicPlayer.bNeedNewBuf) {
		ticks[3]++;
		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		//MusicPlayer_PopulateBuffer(&musicPlayer);
	}

	if (bCountEnable) {
		if(count_1 ==1000){
			sec++;
			count_1 = 0;
		}
		count_1++;
	}

	if (g_cooldownRecord > 0)
		--g_cooldownRecord;
	if (g_cooldownLCD > 0)
		--g_cooldownLCD;
}
void _My_TIM2_IRQHandler() {
	ticks[0]++;
	MusicPlayer_IT_SendPCMToDAC(&musicPlayer);
}
void _My_TIM6_IRQHandler() {
	ticks[1]++;
	MusicPlayer_IT_LoadKeys(&musicPlayer);
}
void _My_TIM7_IRQHandler() {
	ticks[2]++;
	MusicPlayer_IT_RecordData(&musicPlayer);

	//UART_PrintF("%d, %d, %d, %d\r\n", ticks[0], ticks[1], ticks[2], ticks[3]);
	memset(ticks, 0, sizeof(ticks));
}

//ADC

static uint32_t g_adcValue;
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	//HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

	MusicPlayer_SetVolumeRate(&musicPlayer, g_adcValue / (float)0xfff);
}

//EXTI

static bool bRecordBtnPressed = false;
void _My_EXTI0_IRQHandler() {
	if (g_cooldownRecord == 0) {
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

		bRecordBtnPressed = true;

		g_cooldownRecord = 200;
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

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

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
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_RNG_Init();
  MX_TIM2_Init();
  MX_DAC_Init();
  MX_DMA_Init();
  MX_TIM7_Init();
  MX_TIM6_Init();
  MX_ADC1_Init();
  MX_SPI5_Init();
  /* USER CODE BEGIN 2 */

	StorageFlash_Init();

	//LCD init
	ILI9341_Init();

	int posSelected = 1;
	int posCopy = 11;
	int isCopy = 0;
	int screen = 0;

	// screen 0 -> LIST PAGE
	// screen 1 -> PLAY PAGE

	bool bRecordMenu = false;

	/*
	SlotMusic_NewEmpty(&s[0]);
	SlotMusic_New(&s[1], 8);
	SlotMusic_New(&s[2], 210);
	for (int i = 3; i < 10; ++i) {
		SlotMusic_NewEmpty(&s[i]);
	}
	*/

	SlotMusic_LoadFromStorage();

	HAL_ADC_Start_DMA(&hadc1, &g_adcValue, 1);
	HAL_TIM_Base_Start_IT(TIMER_1000HZ);

	MusicPlayer_New(&musicPlayer);

	HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

	/*
	MusicPlayer_SetIsRecording(&musicPlayer, true);
	musicPlayer.bEnableSave = false;

	MusicPlayer_Start(&musicPlayer, 1);
	*/

	/*
	//Music record test
	if (!true) {
		MusicPlayer_SetIsRecording(&musicPlayer, true);
		MusicPlayer_Start(&musicPlayer, 1);

		//HAL_Delay(5000);
		HAL_Delay(1000000);

		MusicPlayer_Stop(&musicPlayer);
		MusicPlayer_FlushRecording(&musicPlayer);
	}

	//Music playback test
	if (true) {
		MusicPlayer_SetIsRecording(&musicPlayer, false);
		MusicPlayer_Start(&musicPlayer, 0);

		while (MusicPlayer_IsPlayback(&musicPlayer))
			HAL_Delay(10);

		;
	}
	*/

	/*
	//FLASH write test
	{
		uint32_t testData[32];
		for (int i = 0; i < 10; ++i) {
			for (int j = 0; j < 32; ++j)
				testData[j] = HAL_RNG_GetRandomNumber(&hrng);
			StorageFlash_Write(MusicPlayer_GetSongAddr(i), sizeof(testData), (const uint32_t*)testData);
		}
	}
	*/

	SetScreen();
	HAL_Delay(1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	if (true) {
		uint16_t x_pos = 0;
		uint16_t y_pos = 0;
		uint16_t position_array[2];
		if (g_cooldownLCD == 0 && TP_Touchpad_Pressed()) {
			if (TP_Read_Coordinates(position_array) == TOUCHPAD_DATA_OK) {
				y_pos = 240 - position_array[0];
				x_pos = position_array[1];

				// Up
				if (x_pos >= 10 && x_pos <= 45 && y_pos >= 70 && y_pos <= 115 &&
						screen == 0) {
					if (posSelected > 0) {
						posSelected -= 1;
					}
				}

				// Down
				else if (x_pos >= 10 && x_pos <= 45 && y_pos >= 120 && y_pos <= 155 &&
						screen == 0) {
					if (posSelected < 9) {
						posSelected += 1;
					}
				}

				// PLAY(GO TO PLAY PAGE)
				else if (x_pos >= 30 && x_pos <= 90 && y_pos >= 200 && y_pos <= 230 &&
						!s[posSelected].empty && !isCopy && screen == 0) {
					screen = 1;
					sec = 0;
					__HAL_TIM_SET_COUNTER(&htim1, 0);
					SetScreen();
				}

				// COPY/PASTE
				else if (x_pos >= 100 && x_pos <= 160 && y_pos >= 200 &&
						y_pos <= 230 && screen == 0)
				{
					ILI9341_Draw_Rectangle(0, 200, 320, 40, BLACK);
					if (!isCopy) {
						//Copy
						posCopy = posSelected;
						isCopy = 1;
					}
					else {
						//Paste
						s[posSelected].Length = s[posCopy].Length;
						s[posSelected].empty = false;
						isCopy = 0;
						posCopy = 11;

						//Save pasted song into memory
						{
							uint32_t* buf = (uint32_t*)musicPlayer.tmpRecordBuffer;

							uint32_t addr = MusicPlayer_GetSongAddr(posCopy);
							StorageFlash_Read(addr, 4, buf);

							uint32_t size = buf[0];
							StorageFlash_Read(addr, size, buf);

							uint32_t addrTo = MusicPlayer_GetSongAddr(posSelected);
							StorageFlash_Write(addrTo, size, (const uint32_t*)buf);
						}
					}
				}

				// CANCEL COPY
				else if (x_pos >= 30 && x_pos <= 90 && y_pos >= 200 && y_pos <= 230 &&
						isCopy && screen == 0) {
					isCopy = 0;
					posCopy = 11;
				}

				// DELETE
				else if (x_pos >= 170 && x_pos <= 230 && y_pos >= 200 &&
						y_pos <= 230 && screen == 0) {
					s[posSelected].empty = true;
					g_cooldownLCD = 500;
				}

				/*
				// PLAY MUSIC
				else if (x_pos >= 30 && x_pos <= 70 && y_pos >= 110 && y_pos <= 150 &&
						isPlay == 0 && screen == 1) {
					isPlay = 1;
					g_cooldownLCD = 500;
				}

				// BACK TO PLAY PAGE
				else if (x_pos >= 240 && x_pos <= 300 && y_pos >= 200 &&
						y_pos <= 230 && screen == 0) {
					screen = 1;
					SetScreen();
				}

				// BACK TO LIST PAGE
				else if (x_pos >= 240 && x_pos <= 300 && y_pos >= 180 &&
						y_pos <= 210 && screen == 1) {
					screen = 0;
					SetScreen();
				}
				*/
			}
		}

		bRecordMenu = false;

		//To play menu, set recording to true
		if (bRecordBtnPressed && screen == 0) {
			screen = 1;
			bRecordMenu = true;
			sec = 0;
			__HAL_TIM_SET_COUNTER(&htim1, 0);
			SetScreen();
		}

		// PAGE LIST OF MUSIC
		if (screen == 0) {

			// UP/DOWN button
			ILI9341_Draw_Rectangle(10, 70, 35, 35, WHITE);
			ILI9341_Draw_Text_Rev("V", 20, 80, BLACK, 2, WHITE);
			ILI9341_Draw_Rectangle(10, 120, 35, 35, WHITE);
			ILI9341_Draw_Text("V", 20, 130, BLACK, 2, WHITE);

			const uint16_t TXT_Y = 208;
			if (!isCopy) {
				uint16_t color = s[posSelected].empty ? WHITE : RED;

				ILI9341_Draw_Rectangle(30, 200, 60, 30, color);
				ILI9341_Draw_Text("PLAY", 40, TXT_Y, BLACK, 2, color);

				ILI9341_Draw_Rectangle(100, 200, 60, 30, color);
				ILI9341_Draw_Text("COPY", 110, TXT_Y, BLACK, 2, color);

				ILI9341_Draw_Rectangle(170, 200, 60, 30, color);
				ILI9341_Draw_Text("ERASE", 180 - 10, TXT_Y, BLACK, 2, color);
			}
			else {
				ILI9341_Draw_Rectangle(30, 200, 60, 30, RED);
				ILI9341_Draw_Text("CANCL", 40 - 10, TXT_Y, BLACK, 2, RED);

				ILI9341_Draw_Rectangle(100, 200, 60, 30, RED);
				ILI9341_Draw_Text("PASTE", 110 - 10, TXT_Y, BLACK, 2, RED);
			}

			{
				int idx = posSelected < 5 ? 0 : 5;
				int sy = 50;
				for (int i = idx; i < idx + 5; i++) {
					uint16_t color = i == posSelected ? YELLOW
						: (i == posCopy ? GREEN : WHITE);
					ILI9341_Draw_Rectangle(60, sy, 230, 20, color);
					//ILI9341_Draw_Text(s[i].Name, 80, sy, BLACK, 2, color);
					if (s[i].empty)
						ILI9341_Draw_Text("Empty", 80, sy, BLACK, 2, color);
					else {
						g_strBuf[0] = '0' + i;
						g_strBuf[1] = '\0';
						ILI9341_Draw_Text(g_strBuf, 80, sy, BLACK, 2, color);
					}
					sy += 30;
				}
			}

			HAL_Delay(50);
		}

		// PLAY PAGE
		else if (screen == 1) {
			ILI9341_Fill_Screen(BLACK);

			// TITLE
			if (!bRecordMenu) {
				sprintf(g_strBuf, "Playing #%d...", posSelected);
			}
			else {
				sprintf(g_strBuf, "Recording #%d...", posSelected);
			}
			ILI9341_Draw_Text(g_strBuf, 64, 50, WHITE, 2, BLACK);

			ILI9341_Draw_Rectangle(30, 110, 40, 40, WHITE);
			if (!bRecordMenu) {
				// PLAY/PAUSE button
				//ILI9341_Draw_Text("PLAY/", 35, 120, BLACK, 1, WHITE);
				//ILI9341_Draw_Text("PAUSE", 35, 130, BLACK, 1, WHITE);
				ILI9341_Draw_Text("PAUSE", 35, 125, BLACK, 1, WHITE);
			}
			else {
				//Stop recording circle sign
				ILI9341_Draw_Filled_Circle(30 + 20, 110 + 20, 16, RED);
			}

			/*
			// Music level range
			ILI9341_Draw_Rectangle(90, 110, 10, 20, WHITE);
			ILI9341_Draw_Rectangle(110, 110, 10, 20, WHITE);
			ILI9341_Draw_Rectangle(130, 110, 10, 20, WHITE);
			ILI9341_Draw_Rectangle(150, 110, 10, 20, WHITE);
			ILI9341_Draw_Rectangle(170, 110, 10, 20, WHITE);
			ILI9341_Draw_Rectangle(190, 110, 10, 20, WHITE);
			ILI9341_Draw_Rectangle(210, 110, 10, 20, WHITE);
			ILI9341_Draw_Rectangle(230, 110, 10, 20, WHITE);
			*/
			ILI9341_Draw_Hollow_Rectangle_Coord(90, 110, 240, 130, WHITE);

			int lenEnd = bRecordMenu ? MAX_SONG_LEN_SEC : s[posSelected].Length;

			//DisplayTimeEx(sec, 80, 140);
			DisplayTimeEx(lenEnd, 200, 140);

			// BACK
			ILI9341_Draw_Rectangle(240, 200, 60, 30, RED);
			ILI9341_Draw_Text("BACK", 245, 210, BLACK, 2, RED);

			{
				MusicPlayer_SetIsRecording(&musicPlayer, bRecordMenu);
				MusicPlayer_Start(&musicPlayer, posSelected);
			}
			bRecordBtnPressed = false;

			//if (isPlay == 1) {
			{
				// start timer

				bCountEnable = true;

				bool bPlaying = true;

				while (1) {
					DisplayTimeEx(sec, bRecordMenu ? 200 : 80, 140);

					float rate = sec / ((float)lenEnd - 1);
					if (rate > 1) rate = 1;
					ILI9341_Draw_Rectangle(90, 110, (240 - 90) * rate, 20, WHITE);

					if (!bRecordMenu) {
						// END
						if (sec >= lenEnd || musicPlayer.playerState == MPLAYER_STOPPED) {
							MusicPlayer_Stop(&musicPlayer);
							ILI9341_Draw_Rectangle(230, 110, 10, 20, RED);

							ILI9341_Draw_Rectangle(30, 110, 40, 40, WHITE);
							ILI9341_Draw_Text("END", 35, 125, RED, 1, WHITE);

							HAL_Delay(1000);

							g_cooldownLCD = 200;
							goto lab_end_menu_1;
						}
					}
					else {
						if (bRecordBtnPressed || sec >= lenEnd) {
							//Save recording and go to song select

							posSelected = 0;
							g_cooldownLCD = 500;

							goto lab_end_menu_1;
						}
					}

					if (g_cooldownLCD == 0 && TP_Touchpad_Pressed()) {
						if (TP_Read_Coordinates(position_array) == TOUCHPAD_DATA_OK) {
							y_pos = 240 - position_array[0];
							x_pos = position_array[1];

							//ILI9341_Draw_Rectangle(x_pos, y_pos, 4, 4, WHITE);

							if (!bRecordMenu) {
								// PAUSE
								if (x_pos >= 30 && x_pos <= 70 && y_pos >= 110 &&
									y_pos <= 150)
								{
									ILI9341_Draw_Rectangle(30, 110, 40, 40, WHITE);
									if (bPlaying) {
										//Now pausing, change to "PLAY"
										ILI9341_Draw_Text("PLAY", 35, 125, BLACK, 1, WHITE);
										MusicPlayer_Pause(&musicPlayer);	//pause music
									}
									else {
										//Now playing, change to "PAUSE"
										ILI9341_Draw_Text("PAUSE", 35, 125, BLACK, 1, WHITE);
										MusicPlayer_Pause(&musicPlayer);	//unpause music
									}
									bPlaying = !bPlaying;
									bCountEnable = bPlaying;
									g_cooldownLCD = 500;
								}

								// BACK TO LIST PAGE
								else if (x_pos >= 240 && x_pos <= 300 && y_pos >= 180 &&
										y_pos <= 210)
								{
									g_cooldownLCD = 500;

									goto lab_end_menu_1;
								}
							}
							else {
								// Stop recording + back
								if (x_pos >= 240 && x_pos <= 300 && y_pos >= 180 &&
										y_pos <= 210)
								{
									g_cooldownLCD = 500;

									goto lab_end_menu_1;
								}
							}
						}
					}

					HAL_Delay(50);
					continue;

lab_end_menu_1:
					screen = 0;
					bCountEnable = false;

					MusicPlayer_Stop(&musicPlayer);
					if (bRecordMenu) {
						uint32_t size = MusicPlayer_FlushRecording(&musicPlayer);

						s[0].Length = CalculateLength(size);
						s[0].empty = false;
					}

					__HAL_TIM_SET_COUNTER(&htim1, 0);

					bRecordBtnPressed = false;
					break;
				}
				ILI9341_Fill_Screen(BLACK);
			}

			HAL_Delay(10);
		}
	}

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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
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
