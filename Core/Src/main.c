/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-17 16:13:25
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-05 10:37:44
 * @Description  : main.c
 */

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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "i2c.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uartConsole.h"
#include "console.h"
#include "bmc.h"
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

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
StackType_t bg_Stack[1024];     /* bgTask静态栈 */
StaticTask_t bg_TaskBuffer;     /* bgTask buffer */
SemaphoreHandle_t uart_mutex;   /* uart访问互斥量 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

void startConsole(void *arg);
void startBackgroundTask(void *arg);
int eeprom_task_func(int arcg, char *argv[]);
int adc_task_func(int argc, char *argv[]);

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
#if 0
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
#endif //! 0
    /* 测试用，eeprom task、ipmi request task、本机adc task */
    Task_t eeprom_task = {
        .task_func = eeprom_task_func,
        .task_name = "eeprom",
        .task_desc = "read or write eeprom to 0x00"};

    Task_t adc_task = {
        .task_func = adc_task_func,
        .task_name = "adc",
        .task_desc = "adc [num], get adc data"};

    /* 初始化uartConsole使用到的硬件 */
    init_hardware();
    /* 初始化uartConsole中的任务以及后台任务队列 */
    init_console_task();
    /* 初始化bmc */
    init_bmc();
    /* 注册测试任务 */
    console_task_register(&eeprom_task);
    console_task_register(&adc_task);

    /* 获取串口访问互斥量实例 */
    uart_mutex = xSemaphoreCreateMutex();

    /* 分别创建uartConsole任务以及后台任务轮询程序, 由于空间分配问题, 后者采用静态创建的方式 */
    xTaskCreate(startConsole, "uartConsole", 512, NULL, 1, NULL);
    xTaskCreateStatic(startBackgroundTask, "bgTask", 1024, NULL, 1, bg_Stack, &bg_TaskBuffer);

    /* USER CODE END 2 */

    /* Call init function for freertos objects (in cmsis_os2.c) */
    MX_FREERTOS_Init();

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
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
    RCC_OscInitTypeDef RCC_OscInitStruct   = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct   = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/***
 * @brief uartConsole task函数
 * @param arg [void*]       该task中不使用arg
 * @return [void]
 */
void startConsole(void *arg)
{
    /* 启动uartConsole */
    console_start();
}

/***
 * @brief bgTask 函数
 * @param arg [void*]   NULL
 * @return [void]
 */
void startBackgroundTask(void *arg)
{
    /* 轮询是否有后台任务需要执行 */
    while (1) {
        run_background_task();
    }
}

/***
 * @brief 测试使用eeprom task函数
 * @param arcg [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        任务执行结果
 */
int eeprom_task_func(int arcg, char *argv[])
{
    uint8_t data[2]   = {0x00};
    uint8_t temp_data = 0;

    /* 失能i2c监听，准备发送 */
    HAL_I2C_DisableListen_IT(&hi2c1);

    if (argv[1][0] == 'w') /* 如果第[1]个参数的第一个字符为 'w'则为写模式 */
    {
        data[1] = (uint8_t)(atoi(argv[2]));
        HAL_I2C_Mem_Write(&hi2c1, 0xa0, 0x00, 1, &(data[1]), 1, 100);
    } else {
        HAL_I2C_Mem_Read(&hi2c1, 0xa0, 0x00, 1, &temp_data, 1, 100);
        PRINTF("read: 0x%02x\r\n", temp_data);
    }

    /* 与eeprom交互完成, 开启监听 */
    HAL_I2C_EnableListen_IT(&hi2c1);

    return 1;
}

/***
 * @brief 本机adc功能测试
 * @param argc [int]    参数数量
 * @param argv [char*]  参数列表
 * @return [int]        任务执行结果
 */
int adc_task_func(int argc, char *argv[])
{
    int ch_count            = 0;    /* 读取的adc通道数 */
    uint16_t p_adc_data[10] = {0};  /* 存放转换后的数据 */

    /* 开启转换，并使用dma传输 */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)p_adc_data, 8);

    /* 判断读取的通道数 */
    if (argc == 1)
        PRINTF("adc c0: %f\r\n", ((float)p_adc_data[0]) / 4096 * 3.3);
    else {
        ch_count = atoi(argv[1]);
        if (ch_count > 8)
            ch_count = 8;

        for (int i = 0; i < ch_count; ++i) {
            PRINTF("adc c%d: %f\r\n", i, ((float)p_adc_data[i]) / 4096 * 3.3);
        }
    }

    return 1;
}

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM7 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM7) {
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
    while (1) {
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
