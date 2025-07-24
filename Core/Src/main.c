/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-17 16:13:25
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-07-24 14:29:35
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
#include "bmc.h"
#include <stdlib.h>
#include "logStore.h"
#include "ipmiHardware.h"
#include <string.h>
#include <stdio.h>
#include "sysInterface.h"
#include "my_task.h"

//TODO: 使用vTaskDelay替换阻塞延时
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
StackType_t req_Stack[880*2];   /* consoleTask静态栈 */
StaticTask_t bg_TaskBuffer;    /* bgTask buffer */
StaticTask_t req_TaskBuffer;     /* consoleTask buffer */
TaskHandle_t writeFile_task;

SemaphoreHandle_t uart_mutex;  /* uart访问互斥量 */

link_list_manager* g_timer_task_manager;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

void sys_req_handler(void *arg);
void startBackgroundTask(void *arg);
void writeFile(void *arg);

static void led_blink(void);
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
    timer_task_t blink = {
        .task_func = led_blink,
        .task_name = "blink",
        .time_interval = 500
    };

    init_sysInterface();

    g_timer_task_manager = init_task_list();
    timer_task_register(g_timer_task_manager, &blink);
    /* 初始化bmc */
    init_bmc();

    // register_fs_ops();

    /* 获取串口访问互斥量实例 */
    uart_mutex = xSemaphoreCreateMutex();

    xTaskCreate(writeFile, "logStoage", 512, NULL, 5, &writeFile_task);

    /* USER CODE END 2 */

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

    HAL_Delay(10);

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
 * @brief task function, 处理系统接口发送的消息
 * @param *arg [void]   参数
 * @return [void]
 */
void sys_req_handler(void *arg)
{
    sys_request_handler();
}

/***
 * @brief bgTask 函数
 * @param arg [void*]   NULL
 * @return [void]
 */
void startBackgroundTask(void *arg)
{
    /* 开启ipmb监听 */
    HAL_I2C_EnableListen_IT(&hi2c1);
    /* 轮询是否有后台任务需要执行 */
    while (1) {
        task_handler(g_timer_task_manager);
    }
}


/*** 
 * @brief log记录任务函数
 * @param arg [void*]   任务参数
 * @return [void]
 */
void writeFile(void *arg)
{
    uint16_t log_file_index = 0;    /* log文件编号记录 */
    char log_file_name[6];          /* log文件名 */
    int f_res;                      /* fs操作结果 */
    lfs_file_t log_file;            /* log文件实例 */
    uint32_t wait_ret;

    /* 初始化该功能使用到的硬件 */
    init_logStore_hardware();
    /* 挂载文件系统并获取生成的log索引 */
    log_file_index = mount_fs();

    /* 成功获取索引，且文件系统成功挂载 */
    if (log_file_index != 0 && fs_flag == 1) {
        sprintf(log_file_name, "%04d", log_file_index);

        /* 创建并打开需要写入的log文件 */
        f_res = lfs_file_open(&lfs, &log_file, log_file_name, LFS_O_WRONLY | LFS_O_CREAT);
        if (f_res) {
            PRINTF("open %s failed\r\n", log_file_name);
        } else {
            PRINTF("open %s success, wait start...\r\n", log_file_name);

        }

        /* 等待开始 */
        wait_ret = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(WAIT_LOG_US));
        if (wait_ret == 1) {
            PRINTF("start\r\n");

            while ((rx_buf_0_last_len != rx_buf_0_len || rx_buf_1_last_len != rx_buf_1_len) && (rx_buf_0_len + rx_buf_1_len) != 0) {
                vTaskDelay(25 / portTICK_PERIOD_MS);
                /* 采用ping-pong buffer，接收和写入不使用同一个buffer */
                if (current_buf == 0) {
                    current_buf       = 1;
                    rx_buf_0_last_len = rx_buf_0_len;
                    rx_buf_0_len      = 0;
                    lfs_file_write(&lfs, &log_file, rx_buf_0, rx_buf_0_last_len);
                } else {
                    current_buf       = 0;
                    rx_buf_1_last_len = rx_buf_1_len;
                    rx_buf_1_len      = 0;
                    lfs_file_write(&lfs, &log_file, rx_buf_1, rx_buf_1_last_len);
                }
                vTaskDelay(25 / portTICK_PERIOD_MS);
            }

            /* 置位结束标志位 */
            end_flag = 1;

            lfs_file_close(&lfs, &log_file);
            PRINTF("write finished\r\n");
        } else {
            //  timeout
            lfs_file_close(&lfs, &log_file);
            PRINTF("no log recv\r\n");
        }

    } else {
        PRINTF("mount fs failed\r\n");
    }

    /* 关闭接收日志串口中断 */
    disable_log_uart();
    /* 分别创建uartConsole任务以及后台任务轮询程序，采用静态创建的方式 */
    xTaskCreateStatic(sys_req_handler, "sysReqHandler", 880 * 2, NULL, 5, req_Stack, &req_TaskBuffer);
    xTaskCreateStatic(startBackgroundTask, "bgTask", 880, NULL, 3, bg_Stack, &bg_TaskBuffer);
    /* 创建接下来的任务后删除当前任务 */
    vTaskDelete(NULL);
}

/*** 
 * @brief 运行指示灯闪烁
 * @return [void]
 */
static void led_blink(void)
{
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);
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
