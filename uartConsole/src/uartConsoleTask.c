/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-01-22 10:41:13
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-11 18:22:25
 * @Description  : 实现任务管理
 */

#include "console.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "uartConsoleTask.h"
#include "hardware.h"

static void free_argv(int argc, char **argv);

/*** 
 * @brief 初始加载的task func，打印帮助信息
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        执行结果
 */
static int help_task(int argc, char *argv[])
{
    PRINTF("\033[1m%-10s%s\033[0m\r\n", "task", "task description");
    for (uint16_t i = 0; i < g_console_task_list->node_number; i++)
    {
        PRINTF("%-10s%s\r\n", ((Task_t *)g_console_task_list->find_by_index(g_console_task_list->list, i))->task_name, ((Task_t *)g_console_task_list->find_by_index(g_console_task_list->list, i))->task_desc);
    }
    
    return 1;
}

static int clear_task(int argc, char *argv[])
{
    CONSOLE_CLEAR();
    return 1;
}

static int led_blink(int argc, char* argv[])
{
    running_led_blink();
    return 1;
}

/* 默认加载任务列表，必须以全为空的成员结尾 */
static Task_t default_task[] = 
{
    {"help", "display help info", help_task},
    {"clear", "clear console", clear_task},
    {"", "", NULL}
};

static Bg_task_t default_bg_task[] = 
{
    {{"led_blink", "led blink task", led_blink}, 500, 0},
    {{"", "", NULL}, 0, 0}
};

/*** 
 * @brief 初始化时加载默认task
 * @return [void]
 */
void add_default_task(void)
{
    uint16_t i = 0;

    while (strlen(default_task[i].task_name) != 0)
    {
        console_task_register(&default_task[i]);
        i++;
    }
    
}

/*** 
 * @brief 初始化时加载默认后台task
 * @return [void]
 */
void add_default_bg_task(void)
{
    uint16_t i = 0;

    while (strlen(default_bg_task[i].task.task_name) != 0)
    {
        console_backgroung_task_register(&default_bg_task[i]);
        i++;
    }
}

/*** 
 * @brief 注册task
 * @param task [Task_t*]    
 * @return [void]
 */
void console_task_register(Task_t* task)
{
    g_console_task_list->add2list(&(g_console_task_list->list), task, sizeof(Task_t), task->task_name, strlen(task->task_name));
}

/*** 
 * @brief 注册后台task
 * @return []
 */
void console_backgroung_task_register(Bg_task_t *bg_task)
{
    bg_task->time_until = bg_task->time_interval + G_TICKS;
    g_console_bg_task_list->add2list(&(g_console_bg_task_list->list), bg_task, sizeof(Bg_task_t), bg_task->task.task_name, strlen(bg_task->task.task_name));
}

/*** 
 * @brief task处理函数，从提交的数据中解析出task以及参数，并调用对应的函数
 * @param submit [uint8_t*]     提交的字符串
 * @param submit_len [uint16_t] 提交的字符串长度
 * @return []
 */
int task_handler(uint8_t *submit, uint16_t submit_len)
{
    int argc = 0;
    char **argv;
    int arg_start = -1;
    int arg_end = 0;
    int temp_argc = 0;
    Task_t *task;

    if (submit_len == 0)
        return 0;

    for (uint16_t i = 0; i < submit_len; i++)   /* 计算参数个数 */
    {
        if (argc == 0)
        {
            if (submit[i] != ' ')
                argc++;
        }
        else
        {
            if ((submit[i] == ' ') && i != (submit_len - 1) && (submit[i+1] != ' '))
                argc++;
        }
    }

    /* 申请对应个数的 char* 空间 */
    argv = (char **)malloc(argc * sizeof(char *));

    for (uint16_t i = 0; i < submit_len; i++)   /* 获取参数，并复制到argv中 */
    {
        if ((submit[i] != ' ') && ((i == 0) || (submit[i-1] == ' ')))
            arg_start = i;

        if ((arg_start != -1) 
            && ((submit[i] != ' ') 
                && ((submit[i+1] == ' ') || (i == (submit_len - 1)))))
        {
            arg_end = i;
            argv[temp_argc] = (char *)malloc(arg_end - arg_start + 1 + 1);
            memset(argv[temp_argc], 0, arg_end - arg_start + 1 + 1);

            memcpy(argv[temp_argc], submit + arg_start, arg_end - arg_start + 1);
            temp_argc++;
        }
    }

    /* 在链表中查找是否存在该task */
    task = g_console_task_list->find_by_id(g_console_task_list->list, argv[0], strlen(argv[0]));
    if (task != NULL)
    {
        task->task_func(argc, argv);
        free_argv(argc, argv);
        return 1;
    }
    else
    {
        free_argv(argc, argv);
        return -1;
    }

}

/*** 
 * @brief 检查任务是否超时，并调用对应回调函数
 * @param task_handle [void*]   任务句柄
 * @return [void]
 */
static void task_timeout_handler(void* task_handle)
{
    Bg_task_t* bg_task = (Bg_task_t*)task_handle;
    if ((G_TICKS >= bg_task->time_until) &&
        ((G_TICKS - 10) <= bg_task->time_until))
    {
        bg_task->task.task_func(0, NULL);
        bg_task->time_until = bg_task->time_interval + G_TICKS;
    }
}

/*** 
 * @brief 遍历链表，检查任务是否超时
 * @return [void]
 */
void bg_task_handler(void)
{
    g_console_bg_task_list->foreach(g_console_bg_task_list->list, task_timeout_handler);
}

/*** 
 * @brief 释放argv
 * @param argc [int]        argv长度
 * @param argv [char**]     参数列表
 * @return []
 */
static void free_argv(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
    {
        free(argv[i]);
    }

    free(argv);
}
