/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-17 16:13:25
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-20 14:29:32
 * @Description  : 
 */
/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-17 16:13:25
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-12 18:05:11
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
#include "bmc.h"
#include <stdlib.h>
#include "logStore.h"
#include "ipmiHardware.h"
#include <string.h>
#include <stdio.h>
#include "ff.h"

//TODO: 使用vTaskDelay替换阻塞延时
//TODO: RAM瓶颈！
//TODO: 测试usart3接收
//TODO: 创建单一FIL实例，并考察sram占用情况
//TODO: 改用littleFS，并考察sram占用情况
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
StackType_t bg_Stack[880];     /* bgTask静态栈 */
StackType_t c_Stack[1024*2];   /* consoleTask静态栈 */
StaticTask_t bg_TaskBuffer;    /* bgTask buffer */
StaticTask_t c_TaskBuffer;     /* consoleTask buffer */

SemaphoreHandle_t uart_mutex;  /* uart访问互斥量 */

uint16_t log_file_index;
extern uint8_t fs_flag;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

void startConsole(void *arg);
void startBackgroundTask(void *arg);
void writeFile(void *arg);

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

    /* 初始化uartConsole使用到的硬件 */
    init_hardware();
    /* 初始化uartConsole中的任务以及后台任务队列 */
    init_console_task();
    /* 初始化bmc */
    init_bmc();

    init_logStore_hardware();
    log_file_index = mount_fs();

    register_fs_ops();

    /* 获取串口访问互斥量实例 */
    uart_mutex = xSemaphoreCreateMutex();

    /* 分别创建uartConsole任务以及后台任务轮询程序, 由于空间分配问题, 后者采用静态创建的方式 */
    xTaskCreateStatic(startConsole, "uartConsole", 1024*2, NULL, 1, c_Stack, &c_TaskBuffer);
    xTaskCreateStatic(startBackgroundTask, "bgTask", 880, NULL, 1, bg_Stack, &bg_TaskBuffer);
    if(log_file_index > 0)
        xTaskCreate(writeFile, "writelog", 768, NULL, 2, NULL);

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
    if (fs_flag)
        PRINTF("mount fs success\r\n");
    else
        PRINTF("mount fs failed\r\n");

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

void writeFile(void *arg)
{
    FIL* log_file;
    char *log_name;
    FRESULT res;

    log_name = pvPortMalloc(5);
    sprintf(log_name, "%04d", log_file_index);

    // FIX: 由于内存瓶颈，这里会申请失败
    log_file = pvPortMalloc(sizeof(FIL));
    if (log_file == NULL)
    {
        PRINTF("generate FIL failed\r\n");
        vPortFree(log_name);
        vTaskDelete(NULL);
    }
    res = f_open(log_file, log_name, FA_WRITE|FA_CREATE_NEW );
    if (res != FR_OK)
    {
        PRINTF("create %s failed\r\n", log_name);
        vPortFree(log_name);
        vTaskDelete(NULL);
    }
    /* 测试用 */
    f_close(log_file);
    vPortFree(log_name);
    vPortFree(log_file);

    vTaskDelete(NULL);
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
