/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file : main.c
  * @PROJECT : Embedded Software Engineer - SPACEIUM
  * @PROGRAMMER: Guillermo Badillo Uribe
  * @FIRST VERSION: 2026-03-18
  * @DESCRIPTION : The Program will recreate a Whack-A-Mole game using LEDS
  * and buttons with freertos we have task that are responsible of the behavior
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdlib.h"
#include "stdio.h"
#include "ssd1331.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    COLOR_BLUE = 0,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_RED
} GameColor_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEBOUNCING_TIME 		250
#define GAME_TIME 				26000
#define OLED_LINE1				5
#define OLED_LINE2				16
#define OLED_LINE3				32
#define OLED_LINE4				45
#define MAX_OLED_STATUS_LENGTH	70
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for oledTask */
osThreadId_t oledTaskHandle;
const osThreadAttr_t oledTask_attributes = {
  .name = "oledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myStartGame */
osThreadId_t myStartGameHandle;
const osThreadAttr_t myStartGame_attributes = {
  .name = "myStartGame",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for countingPoints */
osThreadId_t countingPointsHandle;
const osThreadAttr_t countingPoints_attributes = {
  .name = "countingPoints",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};
/* Definitions for myCounterTask */
osThreadId_t myCounterTaskHandle;
const osThreadAttr_t myCounterTask_attributes = {
  .name = "myCounterTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myGameQueue */
osMessageQueueId_t myGameQueueHandle;
const osMessageQueueAttr_t myGameQueue_attributes = {
  .name = "myGameQueue"
};
/* Definitions for myTimer01 */
osTimerId_t myTimer01Handle;
const osTimerAttr_t myTimer01_attributes = {
  .name = "myTimer01"
};
/* Definitions for timerBinarySem */
osSemaphoreId_t timerBinarySemHandle;
const osSemaphoreAttr_t timerBinarySem_attributes = {
  .name = "timerBinarySem"
};
/* Definitions for oledSemaphore */
osSemaphoreId_t oledSemaphoreHandle;
const osSemaphoreAttr_t oledSemaphore_attributes = {
  .name = "oledSemaphore"
};
/* USER CODE BEGIN PV */
uint32_t turnTime = 1000;
uint8_t prevNum = 4;
__IO uint8_t startGameflag = 0;
__IO uint8_t startTimerFlag = 0;

typedef struct OLED_HandleTypeDef
{
  uint8_t      	points;  // game points
  char			time[20];	// last time to be sent to OLED task
} OLED_HandleTypeDef;

static OLED_HandleTypeDef OLED_State = {
    .points = 0,
    .time   = "00:00:00"
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
void StartDefaultTask(void *argument);
void startOledTask(void *argument);
void startGame(void *argument);
void startCountingPoints(void *argument);
void StartCounterTask(void *argument);
void selectLEDCallback(void *argument);

/* USER CODE BEGIN PFP */
void createTimeString( uint32_t ms, char *timeString, size_t lengthOfTimeString );
void turnOffLED();
void addTotheQueue(uint16_t value);
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
  MX_USART2_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  srand(HAL_GetTick());
  ssd1331_init();
  ssd1331_clear_screen(BLACK);
  ssd1331_draw_rect(0, 0, 90, 60, GREEN); // Green Square
  ssd1331_display_string(5, OLED_LINE1, "Whack-A-Mole!", FONT_1206, WHITE);
  ssd1331_display_string(5, OLED_LINE2, "Press Start!", FONT_1206, WHITE);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of timerBinarySem */
  timerBinarySemHandle = osSemaphoreNew(1, 1, &timerBinarySem_attributes);

  /* creation of oledSemaphore */
  oledSemaphoreHandle = osSemaphoreNew(4, 0, &oledSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of myTimer01 */
  myTimer01Handle = osTimerNew(selectLEDCallback, osTimerOnce, NULL, &myTimer01_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of myGameQueue */
  myGameQueueHandle = osMessageQueueNew (1, sizeof(uint16_t), &myGameQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of oledTask */
  oledTaskHandle = osThreadNew(startOledTask, NULL, &oledTask_attributes);

  /* creation of myStartGame */
  myStartGameHandle = osThreadNew(startGame, NULL, &myStartGame_attributes);

  /* creation of countingPoints */
  countingPointsHandle = osThreadNew(startCountingPoints, NULL, &countingPoints_attributes);

  /* creation of myCounterTask */
  myCounterTaskHandle = osThreadNew(StartCounterTask, NULL, &myCounterTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BLUE_LED_Pin|GREEN_LED_Pin|YELLOW_LED_Pin|SSD1331_CS_Pin
                          |SSD1331_RES_Pin|SSD1331_DC_Pin|RED_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BLUE_LED_Pin GREEN_LED_Pin YELLOW_LED_Pin SSD1331_CS_Pin
                           SSD1331_RES_Pin SSD1331_DC_Pin RED_LED_Pin */
  GPIO_InitStruct.Pin = BLUE_LED_Pin|GREEN_LED_Pin|YELLOW_LED_Pin|SSD1331_CS_Pin
                          |SSD1331_RES_Pin|SSD1331_DC_Pin|RED_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA9 PA10 PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

// FUNCTION      : createTimeString
// DESCRIPTION   : Create a string of the format MM:SS:TT where TT is tenth's of seconds to the provided buffer
// PARAMETERS    : uint32_t ms - current number of ticks to be formatted and output to OLED
//                 char *timeString - pointer to the buffer to store the formatted string to
//                 size_t lengthOfTimeString - maximum size of the buffer to store the formatted string to
//
// RETURNS       : nothing

void createTimeString( uint32_t ms, char *timeString, size_t lengthOfTimeString ){
    if (lengthOfTimeString == 0)
    	return;
    if (lengthOfTimeString < 9) {
    	timeString[0] = '\0';
    	return;
    }  // needs "MM:SS.T" + '\0' = 8
    uint32_t minutes = ms / 60000U;
    uint32_t seconds = (ms % 60000U) / 1000U;
    uint32_t tenths  = (ms % 1000U)  / 100U;
    snprintf(timeString, lengthOfTimeString, "%02lu:%02lu.%1lu", minutes, seconds, tenths);
}

// FUNCTION      : addTotheQueue
// DESCRIPTION   : This Function adds elements to the Queue
// PARAMETERS    : uint16_t value value to add
// RETURNS       : nothing
void addTotheQueue(uint16_t value){
	osStatus_t status = osMessageQueuePut(myGameQueueHandle, &value, 0, 0);
	if (status != osOK) {
	    printf("Full QUEUE");
	}
}

// FUNCTION      : HAL_GPIO_EXTI_Callback
// DESCRIPTION   : This Function has the interruption behavior
// PARAMETERS    : uint16_t GPIO_PIN  pin which interruption is coming from
// RETURNS       : nothing
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN) {

	static uint32_t lastTime = 0;
	uint32_t currentTime = HAL_GetTick();

	if ((currentTime - lastTime) < DEBOUNCING_TIME) {
		return;
	}
	lastTime = currentTime;

    if (GPIO_PIN == GPIO_PIN_8) {
        printf("Blue Button selected\r\n");
        addTotheQueue(COLOR_BLUE);
    }else if (GPIO_PIN == GPIO_PIN_9) {
    	printf("Green Button selected\r\n");
    	addTotheQueue(COLOR_GREEN);
    }else if (GPIO_PIN == GPIO_PIN_10) {
    	printf("Yellow Button selected\r\n");
    	addTotheQueue(COLOR_YELLOW);
    }else if (GPIO_PIN == GPIO_PIN_11) {
    	printf("Red Button selected\r\n");
    	addTotheQueue(COLOR_RED);
    }else if (GPIO_PIN == GPIO_PIN_13) {
    	if(startGameflag == 0){
    		startGameflag = 1;
    		startTimerFlag = 1;
    		printf("Start Button selected\r\n");
    	}
    }
}

// FUNCTION      : turnOffLED
// DESCRIPTION   : This Function turns off all LEDS
// PARAMETERS    : nothing
// RETURNS       : nothing
void turnOffLED(){

	HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);

}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */


  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_startOledTask */
// TASK	         : startOledTask
// DESCRIPTION   : Task to update the OLED according to the structure parameters
// PARAMETERS    : void *argument pointer to the arguments
// RETURNS       : nothing
/* USER CODE END Header_startOledTask */
void startOledTask(void *argument)
{
  /* USER CODE BEGIN startOledTask */
  /* Infinite loop */
  for(;;)
  {
	 //Semaphore to change OLED
	 osSemaphoreAcquire(oledSemaphoreHandle,  osWaitForever);

	 ssd1331_display_string(5, OLED_LINE3, OLED_State.time, FONT_1206, WHITE);
	 ssd1331_display_string(5, OLED_LINE4, "Score:", FONT_1206, WHITE);
	 ssd1331_display_num(40, OLED_LINE4, OLED_State.points , 2, FONT_1206, WHITE);

  }
  /* USER CODE END startOledTask */
}

/* USER CODE BEGIN Header_startGame */
// TASK	         : startGame
// DESCRIPTION   : Task to configured game and start game behavior also reset when finishes
// PARAMETERS    : void *argument pointer to the arguments
// RETURNS       : nothing
/* USER CODE END Header_startGame */
void startGame(void *argument)
{
  /* USER CODE BEGIN startGame */
  /* Infinite loop */
  for(;;)
  {
	if(startGameflag == 1){

		osSemaphoreRelease(oledSemaphoreHandle);


		uint32_t startingTime = HAL_GetTick();
		uint32_t currentTime =  HAL_GetTick();

		while(currentTime - startingTime < GAME_TIME){

			// Starting Timer
			osTimerStart(myTimer01Handle, turnTime);

			//Waiting to the timer finish
			osSemaphoreAcquire(timerBinarySemHandle, osWaitForever);

			// Update the current time
			currentTime = HAL_GetTick();

			turnTime -= 20;
		}

		startGameflag = 0;
		turnOffLED();
		turnTime = 1000;
		prevNum = 4;
		OLED_State.points = 0;
	}
    osDelay(10);
  }
  /* USER CODE END startGame */
}

/* USER CODE BEGIN Header_startCountingPoints */
// TASK	         : startCountingPoints
// DESCRIPTION   : Task to add or substract points
// PARAMETERS    : void *argument pointer to the arguments
// RETURNS       : nothing
/* USER CODE END Header_startCountingPoints */
void startCountingPoints(void *argument)
{
  /* USER CODE BEGIN startCountingPoints */
	uint16_t receivedValue = 0;
  /* Infinite loop */
  for(;;)
  {
    if(startGameflag == 1){
    	osMessageQueueGet(myGameQueueHandle, &receivedValue, NULL, osWaitForever);
    	if(receivedValue == prevNum){
    		printf("Correct Color\r\n");
    		OLED_State.points++;
    	}else{
    		printf("Wrong Color, the led is %d you selected %d\r\n",prevNum, receivedValue);
    		if(OLED_State.points > 0){
    			OLED_State.points--;
    		}
    	}
    	osSemaphoreRelease(oledSemaphoreHandle);

    }
  }
  /* USER CODE END startCountingPoints */
}

/* USER CODE BEGIN Header_StartCounterTask */
// TASK	         : StartCounterTask
// DESCRIPTION   : Task to start the time counter
// PARAMETERS    : void *argument pointer to the arguments
// RETURNS       : nothing
/* USER CODE END Header_StartCounterTask */
void StartCounterTask(void *argument)
{
  /* USER CODE BEGIN StartCounterTask */
  /* Infinite loop */
  for(;;)
  {
	  if(startTimerFlag == 1){

	      	uint32_t  timeleft = GAME_TIME;

	      	while(timeleft > 0){

	      		if(timeleft < GAME_TIME - 1000){

	      			createTimeString(timeleft, OLED_State.time, sizeof(OLED_State.time));

	      			osSemaphoreRelease(oledSemaphoreHandle);
	      		}

	      		timeleft -=100;

	      		osDelay(pdMS_TO_TICKS(100));
	      	}

	      	createTimeString(0, OLED_State.time, sizeof(OLED_State.time));
	      	osSemaphoreRelease(oledSemaphoreHandle);
	      	startTimerFlag = 0;
	      }
	  osDelay(100);
  }
  /* USER CODE END StartCounterTask */
}

/* selectLEDCallback function */
// TASK	         : selectLEDCallback
// DESCRIPTION   : Function to turn on the LED every time the timer finish
// PARAMETERS    : void *argument pointer to the arguments
// RETURNS       : nothing
void selectLEDCallback(void *argument)
{
  /* USER CODE BEGIN selectLEDCallback */
	uint8_t newNum = 0;

	switch(prevNum){
		case COLOR_BLUE:
			HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);
			break;
		case COLOR_GREEN:
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
			break;
		case COLOR_YELLOW:
			HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_RESET);
			break;
		case COLOR_RED:
			HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
			break;
		case 4:
			printf("Initializing the LEDs\r\n");
			break;
		default:
			printf("Something Wrong Happened\r\n");
	}

	do {
		newNum = rand() % 4;
	} while (newNum == prevNum);

	switch(newNum){
			case COLOR_BLUE:
				HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_SET);
				break;
			case COLOR_GREEN:
				HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
				break;
			case COLOR_YELLOW:
				HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_SET);
				break;
			case COLOR_RED:
				HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
				break;
			default:
				printf("Something Wrong Happened\r\n");
		}

	prevNum = newNum;

	osSemaphoreRelease(timerBinarySemHandle);
  /* USER CODE END selectLEDCallback */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM5 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
#ifdef USE_FULL_ASSERT
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
