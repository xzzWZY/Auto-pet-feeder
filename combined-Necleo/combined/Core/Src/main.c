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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "HX711.h"
#include "stdio.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "ssd1306_conf.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RCC_ADDR 0x40021000
#define RCC_AHB2ENR_OFFSET 0x4C
#define RCC_APB1ENR1_OFFSET 0X58
#define GPIOB_ADDR 0X48000400
#define GPIO_MODER_OFFSET 0X00
#define GPIO_AFRL_OFFSET 0X20
#define GPIO_PUPDR_OFFSET 0X0C

#define TIM4_ADDR 0x40000800//timer 4 base register
#define TIM_CR1_OFFSET 0x00//control register 1
#define TIM_PSC_OFFSET 0x28//prescaler
#define TIM_ARR_OFFSET 0x2C// auto reload value
#define TIM_CCMR1_OFFSET 0x18//capture compare mode register
#define TIM_CCER_OFFSET 0x20//capture/compare enable register
#define TIM_CCR2_OFFSET 0x38//capture/compare register 2
#define CCR_MASK 0xFFFF
#define PAGENUM 2
#define HISTORY 20
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
hx711_t food;
hx711_t pet;
hx711_t water;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */
// aht10
uint8_t AHT10_RX_Data[6];
uint32_t AHT10_ADC_Raw;
float AHT10_Temperature, AHT10_Humidity;
uint8_t AHT10_TmpHum_Cmd = 0xAC;
#define AHT10_Adress 0x38 << 1
// load cell
float pet_weight[3] = {0,0,0};
float petweight = 0;
float consumed_food = 0; // food consumed in one day, remember to substract remaining food when a day ends and set it to remaining food weight
// after transferring the data, add the remaining food weight to consumed_food, just before the next transmit, subtract the food weight.
float consumed_water = 0;
float history_food[HISTORY];
float history_water[HISTORY];
float history_pet[HISTORY];
float history_temp[HISTORY];
float history_hum[HISTORY];

// keypad
GPIO_InitTypeDef GPIO_InitStructPrivate = {0};
uint32_t previousMillis = 0;
uint32_t currentMillis = 0;
uint8_t keyPressed = 0;
uint8_t is_press = 0;
// status
uint8_t status = 0; // 0: menu, 1: calibration, 2: measurement
uint8_t dirty = 0;
uint8_t page = 0;
uint8_t disPage = 0;
uint8_t hisPage = 0;
uint8_t line[32];
uint8_t petWarning = 0;
double foodFresh = 100;
double waterFresh = 100;
int8_t warningInfo[2] = {0,0};
//timer
TIM_HandleTypeDef htim5;
int timer5_food_count = -1;
int timer5_water_count = -1;
uint8_t buffer[32]={0};
uint8_t ask_for_warning[32]={'%'};

HAL_StatusTypeDef ret;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	uint32_t * tim4_ccr2 = (uint32_t *)(TIM4_ADDR + TIM_CCR2_OFFSET);

	for (int i = 0; i < 32; i++) {
		line[i] = 0x20;  // Assign space character to each element
	}

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
  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_I2C1_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_DMA(&huart2, buffer, 32);

	HAL_TIM_Base_Start_IT(&htim5);
	hx711_init(&food, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_0); // clk: c1 data: c0
	hx711_init(&water, GPIOG, GPIO_PIN_1, GPIOG, GPIO_PIN_0); // clk: g1 data: g0
	hx711_init(&pet, GPIOC, GPIO_PIN_5, GPIOC, GPIO_PIN_4); // clk: c5 data: c4
	set_scale(&food, 1, 1);
	food.Aoffset = 16190;
	food.Ascale = -722.879456;
	set_scale(&water, 1, 1);
	water.Aoffset = 43813;
	water.Ascale = -741.790161;
	set_scale(&pet, 1, 1);
	pet.Aoffset = 274649;
	pet.Ascale = -53.3035698;

	// initial interrupt
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, 1);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 1);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 1);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, 1);

	// display
	char output;
	char buf[20];
	ssd1306_Init();
	ssd1306_SetCursor(2,2);
	output = ssd1306_WriteString(" Initializing... ", Font_7x10, White);
	ssd1306_UpdateScreen();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	ssd1306_Init();
	dirty = 1;
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);


	while (1)
	{
		if (petWarning)
		{
			uint8_t y = 28;
			if(dirty)
			{
				ssd1306_Init();
				dirty = 0;
			}
			ssd1306_SetCursor(2,2);
			if (petWarning == 1) ssd1306_WriteString("   SUGGESTION   ", Font_7x10, White);
			else ssd1306_WriteString("  !!!WARNING!!!   ", Font_7x10, White);
			ssd1306_SetCursor(2,16);
			ssd1306_WriteString("Yesterday your pet", Font_7x10, White);
			if (warningInfo[0] == 1)
			{
				ssd1306_SetCursor(2,y);
				ssd1306_WriteString("Ate too much!", Font_7x10, White);
				y += 12;
			}
			else if (warningInfo[0] == -1)
			{
				ssd1306_SetCursor(2,y);
				ssd1306_WriteString("Ate too little!", Font_7x10, White);
				y += 12;
			}
			warningInfo[0] = 0;
			if (warningInfo[1] == 1)
			{
				ssd1306_SetCursor(2,y);
				ssd1306_WriteString("Drank too much!", Font_7x10, White);
				y += 12;
			}
			else if (warningInfo[1] == -1)
			{
				ssd1306_SetCursor(2,y);
				ssd1306_WriteString("Drank too little!", Font_7x10, White);
				y += 12;
			}
			ssd1306_SetCursor(2,52);
			ssd1306_WriteString("ENTER-Continue", Font_7x10, White);
			ssd1306_UpdateScreen();
			warningInfo[1] = 0;
			if (is_press && keyPressed == 35)
			{
				petWarning = 0;
				is_press = 0;
				dirty = 1;
			}
		}
		else if (foodFresh < 0 || waterFresh < 0)
		{
			if (dirty)
			{
				ssd1306_Init();
				dirty = 0;
			}
			uint8_t y = 16;
			if (foodFresh < 0)
			{
				ssd1306_SetCursor(2,y);
				ssd1306_WriteString("Food not fresh!", Font_7x10, White);
				y += 12;
			}
			if (waterFresh < 0)
			{
				ssd1306_SetCursor(2,y);
				ssd1306_WriteString("Water not fresh!", Font_7x10, White);
				y += 12;
			}
			ssd1306_SetCursor(2,52);
			ssd1306_WriteString("ENTER-Continue", Font_7x10, White);
			ssd1306_UpdateScreen();
			if (is_press && keyPressed == 35)
			{
				if (foodFresh < 0)foodFresh = 100;
				if (waterFresh < 0)waterFresh = 100;
				is_press = 0;
			}
		}
		else
		{
			if (is_press)
			{
				if (status == 0 && !dirty)
				{
					if (keyPressed == 35)
					{
						page = (page + 1) % PAGENUM;
					}
				}
				if (status == 4 && !dirty)
				{
					if (keyPressed == 35)
					{
						disPage = (disPage + 1) % 3;
					}
				}
				if (status == 5 && !dirty)
				{
					if (keyPressed == 49) // 1: food
					{
						calibrate(&food);
						status = 0;
					}
					else if (keyPressed == 50) //2: water
					{
						calibrate(&water);
						status = 0;
					}
					else if (keyPressed == 51) //3. pet
					{
						calibrate(&pet);
						status = 0;
					}
				}
				if (status == 6 && !dirty)
				{
					if (keyPressed == 35)
					{
						hisPage = (hisPage + 1) % HISTORY;
					}
					else if (keyPressed == 42)
					{
						hisPage = (hisPage + HISTORY - 1) % HISTORY;
					}
				}
				switch (keyPressed)
				{
				case 65: // A
					break;
				case 66: // B
					HAL_UART_Transmit(&huart2, ask_for_warning, 32, 500);
					HAL_Delay(15000);
					break;
				case 67: // C
					//				HAL_UART_Transmit(&huart2, line, 32, 500);
					//				HAL_UART_Transmit(&huart2, data, 32, 500);

					HAL_Delay(2000);
					//				ret = HAL_I2C_Slave_Receive(&hi2c2, buffer, 32, 400);
					break;
				case 68: // D
					status = 0;
					break;
				case 48: // 0

					break;
				case 49: // 1
					status = 1;
					break;
				case 50: // 2
					status = 2;
					break;
				case 51: // 3
					status = 3;
					break;
				case 52: // 4
					status = 4;
					break;
				case 53: // 5
					status = 5;
					break;
				case 54: // 6
					status = 6;
					hisPage = 0;
					break;
				case 55: // 7
					status = 7;
					break;
				case 56: // 8
					break;
				case 57: // 9

					break;
				case 42: // *

					break;
				case 35: // #

					break;
				}
				dirty = 1;
				is_press = 0;
				keyPressed = 0;
			}

			if (status == 0) {
				display_menu();
				pet_weight[2] = pet_weight[1];
				pet_weight[1] = pet_weight[0];
				pet_weight[0] = get_weight(&pet, 10, CHANNEL_A); //get weight
				float average = (pet_weight[2] + pet_weight[1] + pet_weight[0]) / 3.0;
				if (((abs(pet_weight[2] - average)+abs(pet_weight[1] - average)+abs(pet_weight[0] - average)) < (average / 10.0)) && average > 50)
					petweight = floor(pet_weight[1]*10)/10.0;
			}

			else if (status == 5) {
				if (dirty)
				{
					ssd1306_Init();
					ssd1306_SetCursor(0,2);
					ssd1306_WriteString("    Calibration    ", Font_7x10, White);
					ssd1306_SetCursor(2,16);
					ssd1306_WriteString("1-Food Load Cell", Font_7x10, White);
					ssd1306_SetCursor(2,28);
					ssd1306_WriteString("2-Water Load Cell", Font_7x10, White);
					ssd1306_SetCursor(2,40);
					ssd1306_WriteString("3-Pet Load Cell", Font_7x10, White);

					ssd1306_UpdateScreen();
					dirty = 0;
				}
			}

			else if (status == 4) {
				if (dirty)
				{
					ssd1306_Init();
					ssd1306_SetCursor(2,2);
					output = ssd1306_WriteString("PET FEEDER STATUS ", Font_7x10, White);
					ssd1306_UpdateScreen();
					dirty = 0;
				}


				pet_weight[2] = pet_weight[1];
				pet_weight[1] = pet_weight[0];
				pet_weight[0] = get_weight(&pet, 10, CHANNEL_A); //get weight
				float average = (pet_weight[2] + pet_weight[1] + pet_weight[0]) / 3.0;
				if (((abs(pet_weight[2] - average)+abs(pet_weight[1] - average)+abs(pet_weight[0] - average)) < (average / 10.0)) && average > 50)
					petweight = floor(pet_weight[1]*10)/10.0;



				if (disPage == 0)
				{
					float food_weight = floor(get_weight(&food, 10, CHANNEL_A) * 10) / 10.0; //get weight
					float water_weight = floor(get_weight(&water, 10, CHANNEL_A) * 10) / 10.0; //get weight
					gcvt(food_weight, 6, buf);
					ssd1306_SetCursor(2,16);
					output = ssd1306_WriteString("Food: ", Font_7x10, White);
					output = ssd1306_WriteString(buf, Font_7x10, White);
					output = ssd1306_WriteString(" g            ", Font_7x10, White);
					gcvt(water_weight, 6, buf);
					ssd1306_SetCursor(2,28);
					output = ssd1306_WriteString("Water: ", Font_7x10, White);
					output = ssd1306_WriteString(buf, Font_7x10, White);
					output = ssd1306_WriteString(" g            ", Font_7x10, White);
					gcvt(petweight, 6, buf);
					ssd1306_SetCursor(2,40);
					output = ssd1306_WriteString("Pet: ", Font_7x10, White);
					output = ssd1306_WriteString(buf, Font_7x10, White);
					output = ssd1306_WriteString(" g            ", Font_7x10, White);
					gcvt(AHT10_Temperature, 3, buf);
					ssd1306_SetCursor(2,52);
					ssd1306_WriteString("ENTER-Next Page", Font_7x10, White);
				}
				else if (disPage == 1)
				{
					HAL_I2C_Master_Transmit(&hi2c1, AHT10_Adress, &AHT10_TmpHum_Cmd, 1, 100);
					HAL_Delay(1000);
					HAL_I2C_Master_Receive(&hi2c1, AHT10_Adress, (uint8_t*)AHT10_RX_Data, 6, 100);
					/* Convert to Temperature in °C */
					AHT10_ADC_Raw = (((uint32_t)AHT10_RX_Data[3] & 15) << 16) | ((uint32_t)AHT10_RX_Data[4] << 8) | AHT10_RX_Data[5];
					AHT10_Temperature = (float)floor(((AHT10_ADC_Raw * 200.00 / 1048576.00) - 50.00) * 10) / 10.0;
					AHT10_ADC_Raw = ((uint32_t)AHT10_RX_Data[1] << 12) | ((uint32_t)AHT10_RX_Data[2] << 4) | (AHT10_RX_Data[3] >> 4);
					AHT10_Humidity = (float)floor((AHT10_ADC_Raw*100.00/1048576.00) * 10) / 10.0;
					gcvt(AHT10_Temperature, 3, buf);
					ssd1306_SetCursor(2,16);
					output = ssd1306_WriteString("Temp: ", Font_7x10, White);
					output = ssd1306_WriteString(buf, Font_7x10, White);
					output = ssd1306_WriteString(" C             ", Font_7x10, White);
					gcvt(AHT10_Humidity, 3, buf);
					ssd1306_SetCursor(2,28);
					output = ssd1306_WriteString("Humidity: ", Font_7x10, White);
					output = ssd1306_WriteString(buf, Font_7x10, White);
					output = ssd1306_WriteString(" %            ", Font_7x10, White);
					ssd1306_SetCursor(2,52);
					ssd1306_WriteString("ENTER-Next Page", Font_7x10, White);
				}
				else if (disPage == 2)
				{
					gcvt(foodFresh, 4, buf);
					ssd1306_SetCursor(2,16);
					output = ssd1306_WriteString("Food Fresh:", Font_6x8, White);
					output = ssd1306_WriteString(buf, Font_7x10, White);
					output = ssd1306_WriteString(" %             ", Font_7x10, White);
					gcvt(waterFresh, 4, buf);
					ssd1306_SetCursor(2,28);
					output = ssd1306_WriteString("Water Fresh:", Font_6x8, White);
					output = ssd1306_WriteString(buf, Font_7x10, White);
					output = ssd1306_WriteString(" %            ", Font_7x10, White);
					ssd1306_SetCursor(2,52);
					ssd1306_WriteString("ENTER-Next Page", Font_7x10, White);
				}
				ssd1306_UpdateScreen();
			}

			else if (status == 1) {
				release_food(tim4_ccr2);
				status = 0;
			}

			else if (status == 2) {
				release_water();
				status = 0;
			}

			else if (status == 3){
				if(dirty)
				{
					ssd1306_Init();
					dirty = 0;
				}
				float food_weight0 = floor(get_weight(&food, 10, CHANNEL_A) * 10) / 10.0;
				float water_weight0 = floor(get_weight(&water, 10, CHANNEL_A) * 10) / 10.0;
				ssd1306_SetCursor(2,16);
				ssd1306_WriteString("ADD FOOD & WATER", Font_7x10, White);
				ssd1306_SetCursor(2,28);
				ssd1306_WriteString("THEN PRESS ENTER", Font_7x10, White);
				ssd1306_UpdateScreen();
				dirty = 1;
				while(1)
				{
					if (is_press && keyPressed == 35)
					{
						float food_weight1 = floor(get_weight(&food, 10, CHANNEL_A) * 10) / 10.0;
						float water_weight1 = floor(get_weight(&water, 10, CHANNEL_A) * 10) / 10.0;
						consumed_food = consumed_food + food_weight1 - food_weight0;
						consumed_water = consumed_water + water_weight1 - water_weight0;
						status = 0;
						break;
					}
					else if (is_press && keyPressed == 68)
					{
						status = 0;
						break;
					}
				}
			}

			else if (status == 6)
			{
				if(dirty)
				{
					ssd1306_Init();
					dirty = 0;
				}
				gcvt(history_food[hisPage], 6, buf);
				ssd1306_SetCursor(2,2);
				output = ssd1306_WriteString("food: ", Font_7x10, White);
				output = ssd1306_WriteString(buf, Font_7x10, White);
				output = ssd1306_WriteString(" g            ", Font_7x10, White);
				gcvt(history_water[hisPage], 6, buf);
				ssd1306_SetCursor(2,16);
				output = ssd1306_WriteString("water: ", Font_7x10, White);
				output = ssd1306_WriteString(buf, Font_7x10, White);
				output = ssd1306_WriteString(" g            ", Font_7x10, White);
				gcvt(history_pet[hisPage], 6, buf);
				ssd1306_SetCursor(2,28);
				output = ssd1306_WriteString("pet: ", Font_7x10, White);
				output = ssd1306_WriteString(buf, Font_7x10, White);
				output = ssd1306_WriteString(" g            ", Font_7x10, White);
				gcvt(history_temp[hisPage], 3, buf);
				ssd1306_SetCursor(2,40);
				output = ssd1306_WriteString("temp: ", Font_7x10, White);
				output = ssd1306_WriteString(buf, Font_7x10, White);
				output = ssd1306_WriteString(" C             ", Font_7x10, White);
				gcvt(history_hum[hisPage], 3, buf);
				ssd1306_SetCursor(2,52);
				output = ssd1306_WriteString("humidity: ", Font_7x10, White);
				output = ssd1306_WriteString(buf, Font_7x10, White);
				output = ssd1306_WriteString(" %            ", Font_7x10, White);
				ssd1306_UpdateScreen();
			}
			else if (status == 7) // store the data
			{
				for (int i= HISTORY - 2; i>=0; i--)
				{
					history_food[i+1] = history_food[i];
					history_water[i+1] = history_water[i];
					history_pet[i+1] = history_pet[i];
					history_temp[i+1] = history_temp[i];
					history_hum[i+1] = 	history_hum[i];
				}
				float food_weight = floor(get_weight(&food, 10, CHANNEL_A) * 10) / 10.0; // weight*10
				float water_weight = floor(get_weight(&water, 10, CHANNEL_A) * 10) / 10.0;
				HAL_I2C_Master_Receive(&hi2c1, AHT10_Adress, (uint8_t*)AHT10_RX_Data, 6, 100);
				AHT10_ADC_Raw = (((uint32_t)AHT10_RX_Data[3] & 15) << 16) | ((uint32_t)AHT10_RX_Data[4] << 8) | AHT10_RX_Data[5];
				float AHT10_Temp = floor(((AHT10_ADC_Raw * 200.00 / 1048576.00) - 50.00) * 10) / 10.0;
				AHT10_ADC_Raw = ((uint32_t)AHT10_RX_Data[1] << 12) | ((uint32_t)AHT10_RX_Data[2] << 4) | (AHT10_RX_Data[3] >> 4);
				float AHT10_Humid = floor((AHT10_ADC_Raw*100.00/1048576.00) * 10) / 10.0;

				//			int petweight0 = floor(petweight * 10);
				//			int consumedfood = floor(consumed_food*10);
				//			int consumedwater = floor(consumed_water*10);
				history_food[0] = consumed_food - food_weight;
				history_water[0] = consumed_water - water_weight;
				history_pet[0] = petweight;
				history_temp[0] = AHT10_Temp;
				history_hum[0] = 	AHT10_Humid;
				consumed_food = food_weight;
				consumed_water = water_weight;
				float foodRatio = (history_food[1] + history_food[2] + history_food[3] + history_food[4] + 0.1)/history_food[0];
				float waterRatio = (history_water[1] + history_water[2] + history_water[3] + history_water[4] + 0.1)/history_water[0];
				if (foodRatio > 1.4*4) {warningInfo[0] = -1; ++petWarning;}
				if (foodRatio > 2*4) {warningInfo[0] = -1; petWarning+=10;}
				if (foodRatio < 0.7*4) {warningInfo[0] = 1; ++petWarning;}
				if (foodRatio < 0.5*4) {warningInfo[0] = 1; petWarning+=10;}
				if (waterRatio > 1.4*4) {warningInfo[1] = -1; ++petWarning;}
				if (waterRatio > 2*4) {warningInfo[1] = -1; petWarning+=10;}
				if (waterRatio < 0.7*4) {warningInfo[1] = 1; ++petWarning;}
				if (waterRatio < 0.5*4) {warningInfo[1] = 1; petWarning+=10;}
				waterFresh = floor((waterFresh - ((AHT10_Temp + 40)*(AHT10_Temp + 40)/60.0))*10) / 10.0;
				foodFresh = floor((foodFresh - ((AHT10_Temp + 40)*(AHT10_Temp + 40)*(AHT10_Humid + 50) / 20000.0))*10)/10.0;
				status = 0;
			}
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00000E14;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00000E14;
  hi2c2.Init.OwnAddress1 = 20;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_EnableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 19;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 3999;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE6 PE7 PE8 PE9 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PF7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC4 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC1 PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC2 PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB2 PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PG0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : PG1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : PE10 PE11 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE12 PE13 PE14 PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF14_TIM15;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC8 PC9 PC10 PC11
                           PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	char char_buffer[32];
	for (int i = 0; i < sizeof(buffer); i++) {
		char_buffer[i] = (char)(buffer[i]);
	}
	ssd1306_Init();
	ssd1306_SetCursor(2,16);
	ssd1306_WriteString(char_buffer, Font_7x10, White);
	ssd1306_UpdateScreen();
	HAL_UART_Receive_DMA(&huart2, buffer, 32);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == htim5.Instance){
		timer5_food_count++;
		timer5_water_count++;
		//		float food_weight = floor(get_weight(&food, 10, CHANNEL_A) * 10) / 10.0;
		//		float water_weight = floor(get_weight(&water, 10, CHANNEL_A) * 10) / 10.0;
		//		if (timer5_food_count > 1 && food_weight < 10)
		if (timer5_food_count > 1)
		{
			//			release_water();
			//			release_food();
			timer5_food_count = 0;
		}
		//		if (timer5_water_count > 1 && water_weight < 10) // maybe change to container's weight
		//		{
		////			release_water();
		//			timer5_water_count = 0;
		//		}
		//		status = 0;
	}
}

void summary()
{
	line[0] = '$';
	line[30] = '\n';
	line[31] = '\0';
	int food_weight = floor(get_weight(&food, 10, CHANNEL_A) * 10); // weight*10
	HAL_I2C_Master_Receive(&hi2c1, AHT10_Adress, (uint8_t*)AHT10_RX_Data, 6, 100);	AHT10_ADC_Raw = (((uint32_t)AHT10_RX_Data[3] & 15) << 16) | ((uint32_t)AHT10_RX_Data[4] << 8) | AHT10_RX_Data[5];
	int AHT10_Temp = floor(((AHT10_ADC_Raw * 200.00 / 1048576.00) - 50.00) * 10);
	AHT10_ADC_Raw = ((uint32_t)AHT10_RX_Data[1] << 12) | ((uint32_t)AHT10_RX_Data[2] << 4) | (AHT10_RX_Data[3] >> 4);
	int AHT10_Humid = floor((AHT10_ADC_Raw*100.00/1048576.00) * 10);
	int petweight0 = floor(petweight * 10);
	int consumedfood = floor(consumed_food*10);
	int consumedwater = floor(consumed_water*10);
	consumedfood = 1343;
	consumedwater = 896;
	encode(1, AHT10_Temp, 3);
	encode(5, AHT10_Humid, 3);
	encode(9, consumedfood, 4);
	encode(14, consumedwater, 4);
	encode(19, petweight0, 6);
}

void encode(int start, int data, int length)
{
	for (int i = start + length - 1; i >= start; --i)
	{
		line[i] = data % 10;
		data /= 10;
	}
}

void display_menu(){
	if (dirty)
	{
		ssd1306_Init();
		dirty = 0;
	}

	ssd1306_SetCursor(0,2);
	ssd1306_WriteString("       MENU        ", Font_7x10, White);
	if (page == 0)
	{
		ssd1306_SetCursor(2,16);
		ssd1306_WriteString("1-Release Food", Font_7x10, White);
		ssd1306_SetCursor(2,28);
		ssd1306_WriteString("2-Release Water", Font_7x10, White);
		ssd1306_SetCursor(2,40);
		ssd1306_WriteString("3-Feed in Bowl", Font_7x10, White);
		ssd1306_SetCursor(2,52);
		ssd1306_WriteString("ENTER-Next Page", Font_7x10, White);
	}
	else if (page == 1)
	{
		ssd1306_SetCursor(2,16);
		ssd1306_WriteString("4-Real-Time Stat", Font_7x10, White);
		ssd1306_SetCursor(2,28);
		ssd1306_WriteString("5-Calibration", Font_7x10, White);
		ssd1306_SetCursor(2,40);
		ssd1306_WriteString("6-Display History", Font_7x10, White);
		ssd1306_SetCursor(2,52);
		ssd1306_WriteString("ENTER-Next Page", Font_7x10, White);
	}
	ssd1306_UpdateScreen();
}

void calibrate(hx711_t *hx711)
{
	int weight = 0;
	int x = 2; // cursor x
	keyPressed = 0;
	long int prevOffset = 0;

	ssd1306_Init();
	ssd1306_SetCursor(2,16);
	ssd1306_WriteString("REMOVE EVERYTHING", Font_7x10, White);
	ssd1306_SetCursor(2,28);
	ssd1306_WriteString("FROM THE FOOD BOWL", Font_7x10, White);
	ssd1306_SetCursor(2,40);
	ssd1306_WriteString("THEN PRESS ENTER", Font_7x10, White);
	ssd1306_UpdateScreen();

	while(1)
	{
		if (is_press && keyPressed == 35)
		{
			ssd1306_Init();
			ssd1306_SetCursor(2,16);
			ssd1306_WriteString("Calibrating...", Font_7x10, White);
			ssd1306_UpdateScreen();
			set_scale(hx711, 1, 1);
			prevOffset = hx711->Aoffset;
			tare(hx711, 50, CHANNEL_A);
			ssd1306_Init();
			ssd1306_SetCursor(2,16);
			ssd1306_WriteString("PUT KNOWN WEIGHT", Font_7x10, White);
			ssd1306_SetCursor(2,28);
			ssd1306_WriteString("AND ENTER THE MASS", Font_7x10, White);
			ssd1306_SetCursor(2,40);
			ssd1306_WriteString("THEN PRESS ENTER", Font_7x10, White);
			ssd1306_UpdateScreen();
			is_press = 0;
			keyPressed = 0;
			break;
		}
		else if (is_press && keyPressed == 68)
		{
			dirty = 1;
			status = 0;
			return;
		}
	}

	while(1)
	{
		if (!is_press)continue;
		if (keyPressed <= 57 && keyPressed >=48)
		{
			ssd1306_SetCursor(x,52);
			char a[4];
			itoa(keyPressed - 48, a, 10);
			ssd1306_WriteString(a, Font_7x10, White);
			ssd1306_UpdateScreen();
			weight = weight * 10 + keyPressed - 48;
			x += 7;
		}
		else if (keyPressed == 68) // D
		{
			hx711->Aoffset = prevOffset;
			dirty = 1;
			status = 0;
			return;
		}
		else if (keyPressed == 42) // * backspace
		{
			x -= 7;
			ssd1306_SetCursor(x,52);
			ssd1306_WriteString(" ", Font_7x10, White);
			ssd1306_UpdateScreen();
			weight = weight / 10;
		}
		else if (keyPressed == 35) // #
		{
			//modify food
			ssd1306_Init();
			ssd1306_SetCursor(2,16);
			ssd1306_WriteString("Calibrating...", Font_7x10, White);
			ssd1306_UpdateScreen();
			int value = get_value(hx711, 50, CHANNEL_A);
			hx711->Ascale = (float)value / (float)weight;
			ssd1306_Init();
			ssd1306_SetCursor(2,16);
			ssd1306_WriteString("Calibration Done", Font_7x10, White);
			ssd1306_UpdateScreen();
			dirty = 1;
			break;
		}
		is_press = 0;
		keyPressed = 0;
	}
}

void release_food(uint32_t *tim4_ccr2)
{
	if(dirty)
	{
		ssd1306_Init();
		dirty = 0;
	}
	ssd1306_SetCursor(2,16);
	float food_weight0 = floor(get_weight(&food, 10, CHANNEL_A) * 10) / 10.0;
	ssd1306_WriteString("Releasing Food...", Font_7x10, White);
	ssd1306_UpdateScreen();
	dirty = 1;

	*tim4_ccr2 &= ~CCR_MASK;
	*tim4_ccr2 |= 318;
	HAL_Delay(50);

	while(1)
	{
		HAL_Delay(1);
		if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1))
		{
			*tim4_ccr2 &= ~CCR_MASK;
			*tim4_ccr2 |= 300;
			break;
		}
	}
	float food_weight1 = floor(get_weight(&food, 10, CHANNEL_A) * 10) / 10.0;
	consumed_food = consumed_food + food_weight1 - food_weight0;
}

void release_water()
{
	if(dirty)
	{
		ssd1306_Init();
		dirty = 0;
	}
	ssd1306_SetCursor(2,16);
	ssd1306_WriteString("Releasing Water...", Font_7x10, White);
	ssd1306_UpdateScreen();
	dirty = 1;
	float water_weight0 = floor(get_weight(&water, 10, CHANNEL_A) * 10) / 10.0;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, 1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
	//	HAL_Delay(1000*30);
	HAL_Delay(10000);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, 0);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
	float water_weight1 = floor(get_weight(&water, 10, CHANNEL_A) * 10) / 10.0;
	consumed_water = consumed_water + water_weight1 - water_weight0;
}


#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//  #define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	currentMillis = HAL_GetTick();
	if (currentMillis - previousMillis > 200) {
		/*Configure GPIO pins : PB6 PB7 PB8 PB9 to GPIO_INPUT*/
		//    GPIO_InitStructPrivate.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
		//    GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
		//    GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
		//    GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
		//    HAL_GPIO_Init(GPIOE, &GPIO_InitStructPrivate);

		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, 1);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, 0);
		if(GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6))
		{
			keyPressed = 68; //ASCII value of D
		}
		else if(GPIO_Pin == GPIO_PIN_7 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7))
		{
			keyPressed = 67; //ASCII value of C
		}
		else if(GPIO_Pin == GPIO_PIN_8 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8))
		{
			keyPressed = 66; //ASCII value of B
		}
		else if(GPIO_Pin == GPIO_PIN_9 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9))
		{
			keyPressed = 65; //ASCII value of A
		}

		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 1);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, 0);
		if(GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6))
		{
			keyPressed = 35; //ASCII value of #
		}
		else if(GPIO_Pin == GPIO_PIN_7 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7))
		{
			keyPressed = 57; //ASCII value of 9
		}
		else if(GPIO_Pin == GPIO_PIN_8 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8))
		{
			keyPressed = 54; //ASCII value of 6
		}
		else if(GPIO_Pin == GPIO_PIN_9 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9))
		{
			keyPressed = 51; //ASCII value of 3
		}

		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 1);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, 0);
		if(GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6))
		{
			keyPressed = 48; //ASCII value of 0
		}
		else if(GPIO_Pin == GPIO_PIN_7 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7))
		{
			keyPressed = 56; //ASCII value of 8
		}
		else if(GPIO_Pin == GPIO_PIN_8 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8))
		{
			keyPressed = 53; //ASCII value of 5
		}
		else if(GPIO_Pin == GPIO_PIN_9 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9))
		{
			keyPressed = 50; //ASCII value of 2
		}

		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 0);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, 1);
		if(GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6))
		{
			keyPressed = 42; //ASCII value of *
		}
		else if(GPIO_Pin == GPIO_PIN_7 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7))
		{
			keyPressed = 55; //ASCII value of 7
		}
		else if(GPIO_Pin == GPIO_PIN_8 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8))
		{
			keyPressed = 52; //ASCII value of 4
		}
		else if(GPIO_Pin == GPIO_PIN_9 && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9))
		{
			keyPressed = 49; //ASCII value of 1
		}

		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, 1);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 1);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 1);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, 1);
		/*Configure GPIO pins : PB6 PB7 PB8 PB9 back to EXTI*/
		GPIO_InitStructPrivate.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStructPrivate.Pull = GPIO_PULLDOWN;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStructPrivate);
		//    if(keyPressed != 0){
		//
		//    }
		//    keyPressed = 0;
		is_press = 1;
		previousMillis = currentMillis;
	}

}
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
