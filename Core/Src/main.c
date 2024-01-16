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
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include "usart_custom.h"
#include "servo_driver.h"
#include "ILI9341.h"
#include "GFX_Color.h"
#include "fonts/font_8x5.h"
#include "distance_sensor.h"
#include "radar.h"
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
servoDriverStruct servoPA6;
distanceSensorStruct distanceSensorPA2;

radarStruct radar;

UART_Custom_HandleTypeDef huartPC;
UART_Queue uartQueuePC;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_DMA_Init();
  MX_TIM16_Init();
  MX_SPI1_Init();
  MX_TIM17_Init();
  MX_TIM14_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
  ILI9341_Init(&hspi1);
  ILI9341_WriteScreen(ILI9341_BLACK);

  servoDriverInit(&servoPA6, 0, 180, 700, 2650);
  distanceSensorInit(&distanceSensorPA2, 58.0f);

  radarInit(&radar, &servoPA6, &htim3, &distanceSensorPA2, &htim14);
  radarMeasureStart(&radar);
  radarServoStart(&radar);

  uartCustomInit(&huartPC, &huart1, MAX_UART_RX_DATA_LEN, MAX_UART_TX_DATA_LEN);
  uartCustomReceiveDMA(&huartPC);
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  double drawScale = 2;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	 uartQueueReceive(&uartQueuePC);
	 uartQueueTransmit(&uartQueuePC, &huartPC);

	 double dist = radarGetMeasure(&radar);
	 double radians = radarGetPosition(&radar);
	 int16_t yk = round(sin(radians) * dist * drawScale);
	 int16_t xk = round(cos(radians) * dist * drawScale);
	 int16_t xp = 0, yp = 0;

	 GFX_DrawLine(xp + ILI9341_TFTWIDTH / 2, yp + 10, xk + ILI9341_TFTWIDTH / 2, yk + 10, ILI9341_GREEN);

	 if(radians > 3.13 || radians <= 0.1)
	 {
		 ILI9341_WriteScreen(ILI9341_WHITE);
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* TIM16_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM16_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM16_IRQn);
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == radar.sensorTim)
	{
		radarTriggerMeasure(&radar);
	}
	else if(htim == radar.servoTim)
	{
		radarServoUpdate(&radar);
	}
}

/* Distance sensor weak overwritten */
void distanceSensorTrigOn(distanceSensorStruct *sensor)
{
	if(sensor == &distanceSensorPA2)
	{
		HAL_GPIO_WritePin(SENSOR_TRIG_GPIO_Port, SENSOR_TRIG_Pin, GPIO_PIN_SET);
	}
}

void distanceSensorDelay(distanceSensorStruct *sensor, uint32_t delay)
{
	if(sensor == &distanceSensorPA2)
	{
		HAL_Delay(delay);
	}
}

void distanceSensorTrigOff(distanceSensorStruct *sensor)
{
	if(sensor == &distanceSensorPA2)
	{
		HAL_GPIO_WritePin(SENSOR_TRIG_GPIO_Port, SENSOR_TRIG_Pin, GPIO_PIN_RESET);
	}
}
/* Distance sensor weak overwritten */

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == SENSOR_ECHO_Pin)
	{
		HAL_TIM_Base_Start(&htim17);
	}
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == SENSOR_ECHO_Pin)
	{
		HAL_TIM_Base_Stop(&htim17);
		distanceSensorGetEcho(&distanceSensorPA2, __HAL_TIM_GET_COUNTER(&htim17));
		__HAL_TIM_SET_COUNTER(&htim17, 0);
	}
}

/* Servo weak overwritten */
void servoDriverStartTimer(servoDriverStruct *servoDriver)
{
	if(servoDriver == &servoPA6)
	{
		HAL_TIM_PWM_Start_IT(&htim16, TIM_CHANNEL_1);
	}
}

void servoDriverStopTimer(servoDriverStruct *servoDriver)
{
	if(servoDriver == &servoPA6)
	{
		HAL_TIM_PWM_Stop_IT(&htim16, TIM_CHANNEL_1);
	}
}

void servoDriverSetPulseWidth(servoDriverStruct *servoDriver)
{
	if(servoDriver == &servoPA6)
	{
		__HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, servoDriver->positionInPulseWidth);
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
