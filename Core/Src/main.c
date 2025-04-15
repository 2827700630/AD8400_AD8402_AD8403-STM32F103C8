/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/*
 * AD840X系列数字电位器驱动库
 * 雪豹  编写  github.com/2827700630
 * 编码UTF-8
 * 说明在AD840X.h文件中
 * 如果您需要在其他项目中使用这个AD840X驱动，只需：
 * 1. 在STM32CubeMX中配置SPI外设和GPIO引脚，SPI开启DMA的方法见AD840X.h文件
 * 2. 拷贝AD840X.c和AD840X.h两个文件
 * 3. 在您的代码中包含AD840X.h头文件（见第38行）
 * 4. 创建AD840X_HandleTypeDef结构体变量并调用AD840X_Init初始化（见第59行）
 * 5. 然后就可以自由使用AD840X_Write函数了(会自动检测是否支持DMA)
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "AD840X.h" //引入AD840X驱动库
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
// 定义AD840X设备句柄
AD840X_HandleTypeDef hAD840X_1; // 第一个设备
AD840X_HandleTypeDef hAD840X_2; // 第二个设备
AD840X_HandleTypeDef hAD840X_3; // 第三个设备(SHDN和RS未连接到STM32)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  // 初始化第一个AD840X数字电位器，使用完整引脚配置
  AD840X_Init(&hAD840X_1, &hspi1, AD840X_CS1_GPIO_Port, AD840X_CS1_Pin);
  AD840X_Config_Pins(&hAD840X_1, AD840X_SHDN1_GPIO_Port, AD840X_SHDN1_Pin,
                    AD840X_RS1_GPIO_Port, AD840X_RS1_Pin);
  
  // 初始化第二个AD840X数字电位器，使用完整引脚配置
  AD840X_Init(&hAD840X_2, &hspi1, AD840X_CS2_GPIO_Port, AD840X_CS2_Pin);
  AD840X_Config_Pins(&hAD840X_2, AD840X_SHDN2_GPIO_Port, AD840X_SHDN2_Pin,
                    AD840X_RS2_GPIO_Port, AD840X_RS2_Pin);
  
  // 初始化第三个AD840X数字电位器，但不配置SHDN和RS引脚
  // 注意：SHDN和RS引脚必须外部接高电平(VDD)以保证正常工作
  AD840X_Init(&hAD840X_3, &hspi1, AD840X_CS3_GPIO_Port, AD840X_CS3_Pin);
  
  // 复位所有设备的通道到中间值（128）
  AD840X_Reset(&hAD840X_1); // 使用RS引脚复位
  AD840X_Reset(&hAD840X_2); // 使用RS引脚复位
  AD840X_Reset(&hAD840X_3); // 使用SPI命令复位（因为RS未连接）
  
  HAL_Delay(1000);
  
  // 设置初始值
  uint8_t resistance_value1 = 0;  // 第一个设备的起始值
  uint8_t resistance_value2 = 64; // 第二个设备的起始值
  uint8_t resistance_value3 = 128; // 第三个设备的起始值
  
  AD840X_Write(&hAD840X_1, AD840X_CHANNEL_1, resistance_value1);
  AD840X_Write(&hAD840X_2, AD840X_CHANNEL_1, resistance_value2);
  AD840X_Write(&hAD840X_3, AD840X_CHANNEL_1, resistance_value3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 控制LED指示灯闪烁
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    // 每2秒增加电阻值
    resistance_value1 += 4;  // 设备1每次增加4个单位
    resistance_value2 += 8;  // 设备2每次增加8个单位
    resistance_value3 += 16; // 设备3每次增加16个单位

    // 处理溢出
    if (resistance_value1 > 252) resistance_value1 = 0;
    if (resistance_value2 > 248) resistance_value2 = 0;
    if (resistance_value3 > 240) resistance_value3 = 0;

    // 设置新的电阻值
    AD840X_Write(&hAD840X_1, AD840X_CHANNEL_1, resistance_value1);
    AD840X_Write(&hAD840X_2, AD840X_CHANNEL_1, resistance_value2);
    AD840X_Write(&hAD840X_3, AD840X_CHANNEL_1, resistance_value3);
    
    // 使用设备2的第二个通道，反向设置
    AD840X_Write(&hAD840X_2, AD840X_CHANNEL_2, 255 - resistance_value2);

    // 每5秒使用一次低功耗模式功能演示
    static uint8_t counter = 0;
    if(++counter >= 3) {
      counter = 0;
      
      // 将设备1设置为低功耗模式
      AD840X_Shutdown(&hAD840X_1, 0);
      HAL_Delay(500);
      
      // 恢复设备1的正常模式
      AD840X_Shutdown(&hAD840X_1, 1);
    }

    // 延时2秒
    HAL_Delay(2000);

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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
