/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-28 18:28:11
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-01 14:45:01
 * @Description  : 
 */

#include "hardware.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "usart.h"

void __USER init_status_led(void)
{
    MX_GPIO_Init();
}

void __USER init_interface_uart(void)
{
    MX_USART1_UART_Init();
}

void __USER enable_uart_interrupt(void)
{
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

int __USER get_interface_uart_it_flag(uint16_t it)
{
    uint32_t isrflags = READ_REG(huart1.Instance->SR);
    return ((isrflags & it) != RESET);
}

unsigned char __USER get_interface_uart_data(void)
{
    uint8_t rc;
    HAL_UART_Receive(&huart1, &rc, 1, 100);
    return rc;
}

void __USER clear_interface_uart_idel_it_flag(void)
{
    uint32_t temp;
    temp = huart1.Instance->SR;
    temp |= huart1.Instance->DR;
    (void) temp;
}

void __USER interface_uart_send_data(uint8_t* data, uint16_t data_len)
{
    HAL_UART_Transmit(&huart1, data, data_len, 100);
}
