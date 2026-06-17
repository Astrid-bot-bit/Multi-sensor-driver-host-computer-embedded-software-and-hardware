/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_i2c.h"
#include "drv_spl07003.h"
#include "drv_spa07005.h"
#include "drv_spa16001.h"
#include <stdio.h>
#include "sensor_manager.h"
#include "led.h"
#include "key.h"
#include "lcd_spi_154.h"
#include "ui.h"
#include "sensor_protocol.h"
#include "usbd_cdc_if.h"
#include "bsp_timer.h"
#include "bsp_dac_adc.h"
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
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE {
    (void)ch;
    return ch;
}
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
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_SPI3_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  MX_TIM6_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
  BSP_DAC_SetVoltage(1.8f);
  HAL_Delay(200);
  LED_Init();
  KEY_Init();
  SPI_LCD_Init();
  UI_Init();
  Proto_Init(); //1.先初始化

  //2. I2C 检测传感器
  if (Sensor_AutoDetect(&hi2c1) == PRODUCT_UNKNOWN) {
      UI_SetSensorType("NO SENSOR");
      UI_UpdatePressure(-1.0f);
      UI_UpdateTemperature(-999.0f);
      while (1) { HAL_Delay(500); }
  }
  HAL_UART_Transmit(&huart1, (uint8_t*)"HELLO\r\n", 7, 100);
  //3. 初始化传感器
  Sensor_Init();
  UI_SetSensorType(Sensor_GetName());
  BSP_Timer_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 2. 处理PC下发的指令
      Sensor_ProcessRxData();

	  // 1. 优先处理CDC连接事件
	  if (g_cdc_connected) {
		  g_cdc_connected = false;
		  g_sensor_status.continuous_mode = false;
		  uint8_t type_byte = (uint8_t)Sensor_GetType();
		  Proto_SendFrame(CMD_GET_PRODUCT_TYPE, &type_byte, 1U);
	  }

	  if (!BSP_Timer_ConsumeTick()) {
		  continue;
	  }
	  // 3. 连续模式发数据
	  SensorData_t data;
	  bool dataValid = (Sensor_Read(&data) == HAL_OK);

	  if (g_sensor_status.continuous_mode && dataValid) {

			  uint8_t buf[14] = {0};

			// [0..3] 压力 U32 ×100，小端
			uint32_t press_u32 = (uint32_t)(data.pressure_pa * 100.0f);
			buf[0] = (press_u32)       & 0xFF;
			buf[1] = (press_u32 >>  8) & 0xFF;
			buf[2] = (press_u32 >> 16) & 0xFF;
			buf[3] = (press_u32 >> 24) & 0xFF;

			// [4..5] 温度 S16 ×100，小端
			int16_t temp_s16 = (int16_t)(data.temperature * 100.0f);
			buf[4] = (temp_s16)      & 0xFF;
			buf[5] = (temp_s16 >> 8) & 0xFF;

			// [6..8] 压力RAW，3字节，小端
			buf[6] = (data.press_raw)       & 0xFF;
			buf[7] = (data.press_raw >>  8) & 0xFF;
			buf[8] = (data.press_raw >> 16) & 0xFF;

			// [9] 状态寄存器
//			buf[9] = data.status_reg;
			buf[9] = 0x00;

			// [10..13] 温度RAW，4字节，小端
			buf[10] = (data.temp_raw)       & 0xFF;
			buf[11] = (data.temp_raw >>  8) & 0xFF;
			buf[12] = (data.temp_raw >> 16) & 0xFF;
			buf[13] = (data.temp_raw >> 24) & 0xFF;

			Proto_SendFrame(CMD_READ_PRESS_TEMP, buf, 14U);

	  }

	  // 4. LCD更新
	  if (dataValid) {
		  UI_UpdatePressure(data.pressure_pa);
		  UI_UpdateTemperature(data.temperature);
		  UI_UpdateSampleRate(data.press_rate);
		  UI_UpdateOversampling(data.press_osr);
	  }
	  UI_UpdateVoltage(BSP_ADC_GetSensorVoltage());
	  UI_UpdateCurrent(BSP_ADC_GetSensorCurrent());

	  // 5. RTC更新
	  RTC_TimeTypeDef time;
	  RTC_DateTypeDef date;
	  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	  UI_UpdateTime(time.Hours, time.Minutes, time.Seconds);



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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
