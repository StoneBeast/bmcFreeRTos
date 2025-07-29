/*
 * @Author       : stoneBeast
 * @Date         : 2025-04-01 13:38:52
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-01 13:47:51
 * @Description  : 
 */

#ifndef __MY_TASK_H
#define __MY_TASK_H

#include <stdint.h>
#include "link_list.h"

#define TASK_NAME_MAX_LEN   16

#define PRINTF(...)

typedef struct {
    char task_name[TASK_NAME_MAX_LEN];
    void (*task_func)(void);
    uint32_t time_interval;             /* 任务执行时间间隔 */
    uint32_t time_until;                /* 任务超时时间 */
}timer_task_t;

link_list_manager* init_task_list(void);
void timer_task_register(link_list_manager *manager, timer_task_t *task);
void task_handler(link_list_manager *manager);

#endif // !__MY_TASK_H
