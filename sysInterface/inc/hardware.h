/*
 * @Author       : stoneBeast
 * @Date         : 2025-01-21 16:29:16
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-01 14:42:23
 * @Description  : 声明需要实现的硬件接口
 */

#ifndef __HARDWARE_H
#define __HARDWARE_H

#include <stdint.h>

#define __USER

#define SYSINTERFACE_IT_RXEN USART_SR_RXNE
#define SYSINTERFACE_IT_IDLE USART_SR_IDLE

void __USER init_status_led(void);
void __USER init_interface_uart(void);
void __USER enable_uart_interrupt(void);

int __USER get_interface_uart_it_flag(uint16_t it);
uint8_t __USER get_interface_uart_data(void);
void __USER clear_interface_uart_idel_it_flag(void);
void __USER interface_uart_send_data(uint8_t *data, uint16_t data_len);

#endif // !__HARDWARE_H
