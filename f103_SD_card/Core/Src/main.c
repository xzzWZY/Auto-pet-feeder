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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t UART2_RX_Buffer[32] = {0};
uint8_t read_from_nucleo[32];
char send_to_nucleo_Y1[32];
char send_to_nucleo_Y2[32];
char send_to_nucleo_Y3[32];
char send_to_nucleo_Y4[32];
char send_to_nucleo_Y5[32];
double foodFresh=100;
double waterFresh=100;
int received=0;
int changed_tank=0;
uint8_t send_warning[32];
uint8_t empty[32];
FATFS       FatFs;                //Fatfs handle
FIL         fil;                  //File handle
FRESULT     fres;                 //Result after operations

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void example_process_SD_card(void);


FRESULT write_SD_card(
		const uint8_t* wr_text,
	    FIL* fp,            /* [OUT] File object to create */
	    const char* path    /* [IN]  File name to be opened */
);
void get_warning(uint8_t* send_warning);
FRESULT read_last_5_line_SD_card(
		const char* rd_text1,
		const char* rd_text2,
		const char* rd_text3,
		const char* rd_text4,
		const char* rd_text5,
	    FIL* fp,            /* [OUT] File object to create */
	    const char* path    /* [IN]  File name to be opened */
);
FRESULT open_append (
    FIL* fp,            /* [OUT] File object to create */
    const char* path    /* [IN]  File name to be opened */
);
FRESULT read_whole_SD(
		const char* rd_text,
	    FIL* fp,            /* [OUT] File object to create */
	    const char* path    /* [IN]  File name to be opened */
);

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
	for (int i=0; i++; i<32){
		send_warning[i]=0x20; // configure the default character as space characters
	}


//	//delay method
//	for(int i = 0; i < 200; i++){
//		for(int j = 0; j < 1000; j++){}
//	}

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
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_DMA(&huart1, read_from_nucleo, 32);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

//	  get_warning(send_warning);

  while (1)
  {
	  if (received==1){
		  get_warning(send_warning); // every time Blue pill receive data from Nulceo, it will recalcuate warning array
		  received=0;
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB0 PB1 PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the UART3 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}


void HAL_Delay(__IO uint32_t Delay)
{
//  uint32_t tickstart = 0;
//  uwTick = 65530;   //
//  tickstart = HAL_GetTick();
//  while((HAL_GetTick() - tickstart) < Delay)
//  {
//  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, 0);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, 0);

	uint8_t ack[32]="Ack456789012345678901234567890\n\0";

	HAL_UART_Transmit(&huart1, ack, 32, 400);
//    HAL_UART_Transmit(&huart1, read_from_nucleo, strlen((char*)read_from_nucleo), 400);
//    HAL_UART_Transmit(&huart1, read_from_nucleo, 32, 400); //32: strlen((char*)read_from_nucleo)
//    HAL_UART_Transmit(&huart1, "\nCallback", strlen("\nCallback"), HAL_MAX_DELAY);


    //Open the file and will write append to the end of the file
	FRESULT fr;
	UINT br, bw;         /* File read/write count */
	char buffer[8];   /* File copy buffer */

	//	fr = open_append(fp, path);

	/* Opens an existing file. If not exist, creates a new file. */
	fr = f_open(&fil, "test.txt",  FA_READ | FA_WRITE | FA_OPEN_ALWAYS);

	if (read_from_nucleo[0]=='*'){ 
		changed_tank=1;
	}
    if (read_from_nucleo[0]=='$'){ // write append SD card
    	uint8_t ack[32]="Ack$ 6789012345678901234567890\n\0";

    	//	write_SD_card(read_from_nucleo,&fil,"test.txt");
    	received=1;
    	if (fr == FR_OK) {
			/* Seek to end of the file to append data */
			fr = f_lseek(&fil, f_size(&fil));
			if (fr != FR_OK)
				f_close(&fil);
		}
		if(fr != FR_OK)
		{
		printf("File creation/open Error : (%i)\r\n", fr);
		}
		printf("Writing data!!!\r\n");
		char char_read_from_nucleo[32];
		for (int i = 0; i < sizeof(read_from_nucleo); i++) {
			char_read_from_nucleo[i] = (char)(read_from_nucleo[i]);
		}
		//write the data
		f_puts(char_read_from_nucleo, &fil);

		//close your file
		f_close(&fil);
    }
    else if (read_from_nucleo[1]=='$'){ // in case that '\0' leads the read_from_nucleo
		//	write_SD_card(read_from_nucleo,&fil,"test.txt");

		if (fr == FR_OK) {
			/* Seek to end of the file to append data */
			fr = f_lseek(&fil, f_size(&fil));
			if (fr != FR_OK)
				f_close(&fil);
		}
		if(fr != FR_OK)
		{
		printf("File creation/open Error : (%i)\r\n", fr);
		}
		printf("Writing data!!!\r\n");
		char char_read_from_nucleo[32];
		for (int i = 1; i < sizeof(read_from_nucleo); i++) {
			char_read_from_nucleo[i-1] = (char)(read_from_nucleo[i]);
		}
		char_read_from_nucleo[31]='\0';
		//write the data
		f_puts(char_read_from_nucleo, &fil);

		//close your file
		f_close(&fil);
	}
    else if (read_from_nucleo[0]=="%"){ // read one line of data (for 1 day) from SD card
    	uint8_t ack[32]="Ack% 6789012345678901234567890\n\0";
		HAL_UART_Transmit(&huart1, send_warning, 32, 400);
	    	HAL_UART_Transmit(&huart1, send_to_nucleo_Y1, 32, 400);
		HAL_UART_Transmit(&huart1, send_to_nucleo_Y2, 32, 400);
		HAL_UART_Transmit(&huart1, send_to_nucleo_Y3, 32, 400);
		HAL_UART_Transmit(&huart1, send_to_nucleo_Y4, 32, 400);
		HAL_UART_Transmit(&huart1, send_to_nucleo_Y5, 32, 400);

    }


    HAL_UART_Receive_DMA(&huart1, read_from_nucleo, 32); // restart the DMA receive
}

void get_warning(uint8_t* send_warning){
	FRESULT fr;
	fr=read_last_5_line_SD_card(send_to_nucleo_Y1,send_to_nucleo_Y2,send_to_nucleo_Y3,send_to_nucleo_Y4,send_to_nucleo_Y5, &fil,"test.txt");

	char tp[4];
	tp[3]='\0';

	float temp1;
	float temp2;
	float temp3;
	float temp4;
	float temp5;

	float humd1;
	float humd2;
	float humd3;
	float humd4;
	float humd5;

	float water1;
	float water2;
	float water3;
	float water4;
	float water5;

	// Calculate to determine if we need to change the WATER or food in tank
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y1[i+1];
	}
	temp1=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y2[i+1];
	}
	temp2=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y3[i+1];
	}
	temp3=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y4[i+1];
	}
	temp4=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y5[i+1];
	}
	temp5=(float)(atoi(tp))/10;
// 	float avg_temp=(temp1+temp2+temp3+temp4+temp5)/5;
	
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y1[i+5];
	}
	humd1=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y2[i+5];
	}
	humd2=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y3[i+5];
	}
	humd3=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y4[i+5];
	}
	humd4=(float)(atoi(tp))/10;
	for (int i = 0; i < 3 ; i++) {
		tp[i] = send_to_nucleo_Y5[i+5];
	}
	humd5=(float)(atoi(tp))/10;
	
	waterFresh = floor((waterFresh - ((temp1 + 40)*(temp1 + 40)/60.0))*10) / 10.0;
	waterFresh = floor((waterFresh - ((temp2 + 40)*(temp2 + 40)/60.0))*10) / 10.0;
	waterFresh = floor((waterFresh - ((temp3 + 40)*(temp3 + 40)/60.0))*10) / 10.0;
	waterFresh = floor((waterFresh - ((temp4 + 40)*(temp4 + 40)/60.0))*10) / 10.0;
	waterFresh = floor((waterFresh - ((temp5 + 40)*(temp5 + 40)/60.0))*10) / 10.0;

	foodFresh = floor((foodFresh - ((temp1 + 40)*(temp1 + 40)*(humd1 + 50) / 20000.0))*10)/10.0;
	foodFresh = floor((foodFresh - ((temp2 + 40)*(temp2 + 40)*(humd2 + 50) / 20000.0))*10)/10.0;
	foodFresh = floor((foodFresh - ((temp3 + 40)*(temp3 + 40)*(humd3 + 50) / 20000.0))*10)/10.0;
	foodFresh = floor((foodFresh - ((temp4 + 40)*(temp4 + 40)*(humd4 + 50) / 20000.0))*10)/10.0;
	foodFresh = floor((foodFresh - ((temp5 + 40)*(temp5 + 40)*(humd5 + 50) / 20000.0))*10)/10.0;
	
	if (foodFresh<0){
		send_warning[0]='T';
	}
	else {
		send_warning[0]='F';
	}
	if (waterFresh<0){
		send_warning[1]='T';
	}
	else {
		send_warning[1]='F';
	}
	if (changed_tank=1){
		if (foodFresh<0){
			foodFresh=100;
		}
		if (waterFresh<0){
			waterFresh=100;
		}
		changed_tank=0;
	}
	
	// Calculate to determine giving DRINGKING too little water WANING
	char tp2[6];
	tp2[5]='\0';
	char tp3[5];
	tp3[4]='\0';
	float pet_weight;
	for (int i = 0; i < 6 ; i++) {
		tp2[i] = send_to_nucleo_Y1[i+19];
	}
	pet_weight=(float)(atoi(tp2))/10;
	float std_drink_water;
	std_drink_water=5*50*pet_weight/1000; //unit is ml

	for (int i = 0; i < 4 ; i++) {
		tp3[i] = send_to_nucleo_Y1[i+14];
	}
	water1=(float)(atoi(tp3))/10;
	for (int i = 0; i < 4 ; i++) {
		tp3[i] = send_to_nucleo_Y2[i+14];
	}
	water2=(float)(atoi(tp3))/10;
	for (int i = 0; i < 4 ; i++) {
		tp3[i] = send_to_nucleo_Y3[i+14];
	}
	water3=(float)(atoi(tp3))/10;
	for (int i = 0; i < 4 ; i++) {
		tp3[i] = send_to_nucleo_Y4[i+14];
	}
	water4=(float)(atoi(tp3))/10;
	for (int i = 0; i < 4 ; i++) {
		tp3[i] = send_to_nucleo_Y5[i+14];
	}
	water5=(float)(atoi(tp3))/10;

	float all_water_drunk=water1+water2+water3+water4+water5;
	if (all_water_drunk<std_drink_water){
		send_warning[2]='T';
	}
	else {
		send_warning[2]='F';
	}
	
	// Calculate to determine giving EATING too MUCH food WANING
	float std_eat_food=pet_weight*0.03;

	float food1;
	for (int i = 0; i < 4 ; i++) {
		tp3[i] = send_to_nucleo_Y1[i+9];
	}
	food1=(float)(atoi(tp3))/10;
	if (food1<std_eat_food){
		send_warning[3]='T';
	}
	else{
		send_warning[3]='F';
	}

	// Calculate to determine giving EATING too LITTLE food WANING
	float stdmore_eat_food=pet_weight*0.05;

	if (food1>stdmore_eat_food){
		send_warning[4]='T';
	}
	else{
		send_warning[4]='F';
	}
	send_warning[5]='\n';
	send_warning[31]='\0';
}

FRESULT read_last_5_line_SD_card(
		const char* rd_text1,
		const char* rd_text2,
		const char* rd_text3,
		const char* rd_text4,
		const char* rd_text5,
		FIL* fp,            /* [OUT] File object to create */
	    const char* path    /* [IN]  File name to be opened */
)
{
	// Mount the SD Card
	fres = f_mount(&FatFs, "", 1);    //1=mount now
	if (fres != FR_OK)
	{
	  printf("No SD Card found : (%i)\r\n", fres);
	}
	//Open the file and will write append to the end of the file
    FRESULT fr;
//    UINT br, bw;         /* File read/write count */


    /* Opens an existing file and read it. */
    fr = f_open(fp, path, FA_READ);
    if (fr == FR_OK) {
        /* Seek to last line of the file to read last line of data */
        fr = f_lseek(fp, f_size(fp)-32);
        if (fr != FR_OK)
            f_close(fp);
    }

	//read the data
    f_gets(rd_text1, 32, fp);

	/* Seek to last 2nd line of the file to read last line of data */
	fr = f_lseek(fp, f_tell(fp)-64);
	if (fr == FR_OK){
		f_gets(rd_text2, 32, fp);
	}

	/* Seek to last 3rd line of the file to read last line of data */
	fr = f_lseek(fp, f_tell(fp)-96);
	if (fr == FR_OK){
		f_gets(rd_text3, 32, fp);
	}

	/* Seek to last 4th line of the file to read last line of data */
	fr = f_lseek(fp, f_tell(fp)-128);
	if (fr == FR_OK){
		f_gets(rd_text4, 32, fp);
	}
	/* Seek to last 5th line of the file to read last line of data */
	fr = f_lseek(fp, f_tell(fp)-160);
	if (fr == FR_OK){
		f_gets(rd_text5, 32, fp);
	}

	//close your file
	f_close(fp);
	//We're done, so de-mount the drive
	f_mount(NULL, "", 0);

	return fr;
}

FRESULT read_whole_SD(
		const char* rd_text,
	    FIL* fp,            /* [OUT] File object to create */
	    const char* path    /* [IN]  File name to be opened */
)
{
	// Mount the SD Card
	fres = f_mount(&FatFs, "", 1);    //1=mount now
	if (fres != FR_OK)
	{
	  printf("No SD Card found : (%i)\r\n", fres);
	}
	//Open the file and will write append to the end of the file
    FRESULT fr;
    UINT br;         /* File read count */


    /* Opens an existing file. If not exist, creates a new file. */
    fr = f_open(fp, path, FA_READ);

	if(fr != FR_OK)
	{
	printf("File creation/open Error : (%i)\r\n", fr);
	}

	printf("Read data!!!\r\n");
	//read the data
//	f_gets(rd_text, sizeof rd_text, fp);
    fr = f_read(fp, rd_text, sizeof rd_text, &br); /* Read a chunk of data from the source file */


	//close your file
	f_close(fp);
	//We're done, so de-mount the drive
	f_mount(NULL, "", 0);

	return fr;
}

FRESULT write_SD_card(
		const uint8_t* wr_text,
	    FIL* fp,            /* [OUT] File object to create */
	    const char* path    /* [IN]  File name to be opened */
)
{
	// Mount the SD Card
	fres = f_mount(&FatFs, "", 1);    //1=mount now
	if (fres != FR_OK)
	{
	  printf("No SD Card found : (%i)\r\n", fres);
	}
	//Open the file and will write append to the end of the file
    FRESULT fr;

//	fr = open_append(fp, path);

    /* Opens an existing file. If not exist, creates a new file. */
    fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
        /* Seek to end of the file to append data */
        fr = f_lseek(fp, f_size(fp));
        if (fr != FR_OK)
            f_close(fp);
    }
	if(fr != FR_OK)
	{
	printf("File creation/open Error : (%i)\r\n", fr);
	}

	printf("Writing data!!!\r\n");

	char char_wr_text[32];
	int size=sizeof(wr_text);
	for (int i = 0; i < 32; i++) {
		char_wr_text[i] = (char)(wr_text[i]);
	}
	//write the data
	f_puts(char_wr_text, fp);

	//close your file
	f_close(fp);
	//We're done, so de-mount the drive
	f_mount(NULL, "", 0);
	return fr;
}

/*------------------------------------------------------------/
/ Open or create a file in append mode
/ (This function was sperseded by FA_OPEN_APPEND flag at FatFs R0.12a)
/------------------------------------------------------------*/

FRESULT open_append (
    FIL* fp,            /* [OUT] File object to create */
    const char* path    /* [IN]  File name to be opened */
)
{
    FRESULT fr;

    /* Opens an existing file. If not exist, creates a new file. */
    fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
        /* Seek to end of the file to append data */
        fr = f_lseek(fp, f_size(fp));
        if (fr != FR_OK)
            f_close(fp);
    }
    return fr;
}

void example_process_SD_card( void )
{
  FATFS       FatFs;                //Fatfs handle
  FIL         fil;                  //File handle
  FRESULT     fres;                 //Result after operations
  char        buf[100];

  do
  {
    //Mount the SD Card
    fres = f_mount(&FatFs, "", 1);    //1=mount now
    if (fres != FR_OK)
    {
      printf("No SD Card found : (%i)\r\n", fres);
      break;
    }
    printf("SD Card Mounted Successfully!!!\r\n");

    //Read the SD Card Total size and Free Size
    FATFS *pfs;
    DWORD fre_clust;
    uint32_t totalSpace, freeSpace;

    f_getfree("", &fre_clust, &pfs);
//    totalSpace = (uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5);
//    freeSpace = (uint32_t)(fre_clust * pfs->csize * 0.5);

    printf("TotalSpace : %lu bytes, FreeSpace = %lu bytes\n", totalSpace, freeSpace);

    //Open the file
    fres = f_open(&fil, "EmbeTronicX.txt", FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if(fres != FR_OK)
    {
      printf("File creation/open Error : (%i)\r\n", fres);
      break;
    }

    printf("Writing data!!!\r\n");
    //write the data
    f_puts("Welcome to EmbeTronicX", &fil);

    //close your file
    f_close(&fil);

    //Open the file
    fres = f_open(&fil, "EmbeTronicX.txt", FA_READ);
    if(fres != FR_OK)
    {
      printf("File opening Error : (%i)\r\n", fres);
      break;
    }

    //read the data
    f_gets(buf, sizeof(buf), &fil);

    printf("Read Data : %s\n", buf);

    //close your file
    f_close(&fil);
    printf("Closing File!!!\r\n");
#if 0
    //Delete the file.
    fres = f_unlink(EmbeTronicX.txt);
    if (fres != FR_OK)
    {
      printf("Cannot able to delete the file\n");
    }
#endif
  } while( false );

  //We're done, so de-mount the drive
  f_mount(NULL, "", 0);
  printf("SD Card Unmounted Successfully!!!\r\n");
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
