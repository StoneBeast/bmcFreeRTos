/*
 * @Author       : stoneBeast
 * @Date         : 2025-01-22 11:18:54
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-17 17:08:41
 * @Description  : task.h
 */

#ifndef __UART_CONSOLE_TASK_H_
#define __UART_CONSOLE_TASK_H_

#include "uartConsoleConfig.h"

/* task结构体 */
typedef struct 
{
    char task_name[TASK_NAME_LEN];              /* task名称 */
    char task_desc[TASK_DESC_LEN];              /* task描述 */
    int (*task_func)(int argc, char* argv[]);   /* task函数 */
}Task_t;

typedef struct
{
    Task_t task;                                /* task */  
    uint32_t time_interval;                     /* 任务执行时间间隔 */
    uint32_t time_until;                        /* 任务超时时间 */
}Bg_task_t;

void add_default_task(void);
void add_default_bg_task(void);
int task_handler(uint8_t* submit, uint16_t submit_len);
void bg_task_handler(void);

#endif // !__UART_CONSOLE_TASK_H_
