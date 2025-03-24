/*
 * @Author       : stoneBeast
 * @Date         : 2025-01-21 16:26:22
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-18 16:15:31
 * @Description  : uartConsole包对外暴露的接口
 */

#ifndef __UART_CONSOLE_H
#define __UART_CONSOLE_H

#include "uartConsoleTask.h"

#define PRINTF(...)             console_printf(__VA_ARGS__)

extern void console_printf(const char *fmt, ...);

void init_hardware(void);
void init_console_task(void);
void console_start(void);
extern void console_task_register(Task_t* task);
extern void console_backgroung_task_register(Bg_task_t* task);
void run_background_task(void);

#endif // !__UART_CONSOLE_H
