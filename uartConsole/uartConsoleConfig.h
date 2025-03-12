/*
 * @Author       : stoneBeast
 * @Date         : 2025-01-21 16:35:23
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-18 16:08:04
 * @Description  : 存放硬件的某些配置项，但配置项也是根据特定的平台来定义的，没有做到真正的完全解耦
 */

#ifndef __UART_CONSOLE_CONFIG_H
#define __UART_CONSOLE_CONFIG_H

#include "stm32f1xx_hal.h"

#define TICKS_PER_SECOND 1000

#define STATUS_LED_PORT GPIOA
#define STATUS_LED_PIN GPIO_PIN_8

#define CONSOLE_UART USART1
#define CONSOLE_UART_IRQ USART1_IRQn
#define CONSOLE_UART_BAUDRATE 115200
#define CONSOLE_IT_RXEN USART_SR_RXNE
#define CONSOLE_IT_IDLE USART_SR_IDLE
#define CONSOLE_UART_IRQ_HANDLER USART1_IRQHandler
#define CONSOLE_TIMEBASE_HANDLER SysTick_Handler

#define TASK_NAME_LEN           32
#define TASK_DESC_LEN           128

#define USE_PRIVATE_TICKS       0

#define G_TICKS         get_ticks()

#endif // !__UART_CONSOLE_CONFIG_H
