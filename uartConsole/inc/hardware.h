/*
 * @Author       : stoneBeast
 * @Date         : 2025-01-21 16:29:16
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-01-24 13:45:33
 * @Description  : 声明需要实现的硬件接口
 */

#ifndef __HARDWARE_H
#define __HARDWARE_H

#include "uartConsoleConfig.h"

#define __USER

void __USER init_timebase(void);
void __USER init_status_led(void);
void __USER init_console_uart(void);
void __USER enable_uart_interrupt(void);

int __USER get_console_uart_it_flag(uint16_t it);
uint8_t __USER get_console_uart_data(void);
void __USER clear_console_uart_idel_it_flag(void);
void __USER console_uart_send_data(uint8_t data);

void __USER inc_ticks(void);
uint32_t __USER get_ticks(void);

void delay_ms(uint32_t ms);
void running_led_blink(void);

#endif // !__HARDWARE_H
