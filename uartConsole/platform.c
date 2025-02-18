/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-01-21 16:30:58
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-18 13:45:35
 * @Description  : 可移植层，针对不同的平台实现规定的硬件api
 */

#include "hardware.h"
#include "uartConsoleconfig.h"
#include "usart.h"
#include "gpio.h"

#if USE_PRIVATE_TICKS
volatile uint32_t g_Ticks = 0;              /* 全局滴答计数 */
#endif  // USE_PRIVATE_TICKS

void __USER init_timebase(void)
{
    // SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);
}

void __USER init_status_led(void)
{
    MX_GPIO_Init();
}

void __USER init_console_uart(void)
{
    MX_USART1_UART_Init();
}

void __USER enable_uart_interrupt(void)
{
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

int __USER get_console_uart_it_flag(uint16_t it)
{
    uint32_t isrflags = READ_REG(huart1.Instance->SR);
    return ((isrflags & it) != RESET);
}

unsigned char __USER get_console_uart_data(void)
{
    uint8_t rc;
    HAL_UART_Receive(&huart1, &rc, 1, 100);
    return rc;
}

void __USER clear_console_uart_idel_it_flag(void)
{
    uint32_t temp;
    temp = CONSOLE_UART->SR;
    temp |= CONSOLE_UART->DR;
}

void __USER console_uart_send_data(uint8_t data)
{
    HAL_UART_Transmit(&huart1, &data, 1, 100);
}

void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void running_led_blink(void)
{
    HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
}

uint32_t __USER get_ticks(void)
{
#if USE_PRIVATE_TICKS
    return g_Ticks;
#else
    // __USER get ticks;
    /* don't use private ticks */
    return HAL_GetTick();
#endif
}

void __USER inc_ticks(void)
{
#if USE_PRIVATE_TICKS
    g_Ticks++;
#else
    // __USER increase ticks;
    /* don't use private ticks */
    HAL_IncTick();
#endif
}


