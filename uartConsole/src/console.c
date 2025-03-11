/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-01-21 16:25:45
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-18 15:56:30
 * @Description  : 串口终端程序的主要逻辑实现
 */

#include "hardware.h"
#include "console.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "link_list.h"
#include <stdlib.h>
#include "cmsis_os.h"

extern SemaphoreHandle_t uart_mutex;
console_struct g_console;                   /* 全局console结构体 */
link_list_manager *g_console_task_list;     /* 全局task链表 */
link_list_manager *g_console_bg_task_list;  /* 全局后台任务链表 */


/*** 
 * @brief 初始化console结构体
 * @return [void]
 */
static void init_console_struct(void)
{
    memset(&g_console, 0, sizeof(console_struct));
}

/*** 
 * @brief 初始化使用到的硬件
 * @return [void]
 */
void init_hardware(void)
{
    init_timebase();
    init_status_led();
    init_console_uart();
    enable_uart_interrupt();
}

/*** 
 * @brief 初始化task机制
 * @return [void]
 */
void init_console_task(void)
{
    // TODO: 从console_start中拆分出来本质是为task_register预留钩子，之后可以修改为其他机制

    /* 获取task链表实例，并注册默认任务 */
    g_console_task_list = link_list_manager_get();
    g_console_bg_task_list = link_list_manager_get();
    add_default_task();
    add_default_bg_task();
}

/*** 
 * @brief   向外暴露的接口，开始遍历并寻找超时的后台任务
 * @return [void]
 */
void run_background_task(void)
{
    bg_task_handler();
}

/*** 
 * @brief 启动主程序
 * @return [void]
 */
void console_start(void)
{
    uint8_t temp_buffer[BUFFER_LEN] = { 0 };    /* 暂时存放接收到的数据 */
    uint16_t temp_len = 0;                      /* 接收的数据的长度 */
    int task_ret = 0;                           /* 接收task执行的结果 */

    /* 初始胡console_struct */
    init_console_struct();

    /* 打印title */
    CONSOLE_OUT("");

    while (1)   /* 循环判断标志位 */
    {
        if (g_console.edit_frame_flag)  /* 一帧数据帧接收完成 */
        {
            /* 马上将数据取出，并复位标志位 */
            memcpy(temp_buffer, g_console.edit_frame, g_console.edit_frame_len+1);
            temp_len = g_console.edit_frame_len;
            g_console.edit_frame_len = 0;
            g_console.edit_frame_flag = 0;

            /* 处理数据 */
            if (temp_len > 1) /* 长度大于1，可能是特殊字符 */
            {
                if (ASSERENT_KEY(temp_buffer, KEY_RIGHT) && (g_console.cursor < g_console.edit_len))    /* 方向右 */
                {
                    g_console.cursor++;
                    CURSOR_RIGHT(1);
                }
                else if (ASSERENT_KEY(temp_buffer, KEY_LEFT) && (g_console.cursor > 0))                 /* 方向左 */
                {
                    g_console.cursor--;
                    CURSOR_LEFT(1);
                }
                else if (ASSERENT_KEY(temp_buffer, KEY_UP))                                             /* UP键 */
                {
                    if ((g_console.history_len > 0) && (0 != strcmp((char*)(g_console.history_buffer), (char*)(g_console.edit_buffer)))) /* 历史记录不为空且当前输入不为空 */
                    {

                        memcpy(g_console.temp_edit_buffer, g_console.edit_buffer, g_console.edit_len+1);
                        g_console.temp_edit_len = g_console.edit_len;

                        memcpy(g_console.edit_buffer, g_console.history_buffer, g_console.history_len+1);
                        g_console.edit_len = g_console.history_len;

                        g_console.cursor = g_console.edit_len;
                        g_console.edit_buffer_changed_flag = 1;
                        g_console.history_flag = 1;
                    }
                }
                else if (ASSERENT_KEY(temp_buffer, KEY_DOWN))                                           /* DOWN键 */
                {
                    if (0 != strcmp((char*)(g_console.temp_edit_buffer), (char*)(g_console.edit_buffer))) /* 临时记录不为空且当前输入不为空 */
                    {
                        memcpy(g_console.history_buffer, g_console.edit_buffer, g_console.edit_len + 1);
                        g_console.history_len = g_console.edit_len;

                        memcpy(g_console.edit_buffer, g_console.temp_edit_buffer, g_console.temp_edit_len+1);
                        g_console.edit_len = g_console.temp_edit_len;

                        g_console.cursor = g_console.edit_len;
                        g_console.edit_buffer_changed_flag = 1;
                        g_console.history_flag = 1;
                    }
                }
                else if (ASSERENT_KEY(temp_buffer, KEY_HOME))                                           /* HOME键 */
                {
                    if (g_console.cursor > 0)
                    {
                        CURSOR_LEFT(g_console.cursor);
                        g_console.cursor = 0;
                    }
                }
                else if (ASSERENT_KEY(temp_buffer, KEY_END))                                            /* END键 */
                {
                    if (g_console.cursor < g_console.edit_len)
                    {
                        CURSOR_RIGHT(g_console.edit_len - g_console.cursor);
                        g_console.cursor = g_console.edit_len;
                    }
                }
                else if (ASSERENT_KEY(temp_buffer, KEY_DELETE) && (g_console.cursor < g_console.edit_len)) /* 删除键 */
                {
                    memmove(g_console.edit_buffer + g_console.cursor, g_console.edit_buffer + g_console.cursor + 1, g_console.edit_len - g_console.cursor);
                    g_console.edit_len--;
                    g_console.edit_buffer[g_console.edit_len] = '\0';
                    g_console.delete_flag = 1;
                    g_console.edit_buffer_changed_flag = 1;
                }
            }
            else if(temp_len == 1)    /* 可能是可视字符 */
            {
                if (temp_buffer[0] >= 0x20 && temp_buffer[0] <= 0x7e) /* 可视字符 */
                {
                    memmove(g_console.edit_buffer + g_console.cursor + 1, g_console.edit_buffer + g_console.cursor, g_console.edit_len - g_console.cursor + 1);
                    g_console.edit_buffer[g_console.cursor] = temp_buffer[0];
                    g_console.edit_len++;
                    g_console.cursor++;
                    g_console.edit_buffer_changed_flag = 1;
                }
                else if (temp_buffer[0] == KEY_BACKSPACE && (g_console.cursor > 0))            /* 退格键 */
                {
                    memmove(g_console.edit_buffer + g_console.cursor - 1, g_console.edit_buffer + g_console.cursor, g_console.edit_len - g_console.cursor + 1);
                    g_console.edit_len--;
                    g_console.edit_buffer[g_console.edit_len] = '\0';
                    g_console.cursor--;
                    CURSOR_LEFT(1);
                    g_console.delete_flag = 1;
                    g_console.edit_buffer_changed_flag = 1;
                }
                else if (temp_buffer[0] == KEY_ENTER) /* 回车键 */
                {
                    /* 将当前的edit_buffer存放到submit_buffer中，并置位edit_flag */
                    memcpy(g_console.submit_buffer, g_console.edit_buffer, g_console.edit_len);
                    g_console.submit_buffer[g_console.edit_len] = '\0';
                    g_console.submit_len = g_console.edit_len;
                    CLEAR_BUFFER(g_console.edit_buffer);
                    g_console.edit_len = 0;
                    g_console.cursor = 0;
                    g_console.edit_flag = 1;
                    CONSOLE_NEW_LINE();
                }
            }

        }
        if (g_console.edit_buffer_changed_flag) /* edit_buffer发生更改 */
        {
            if (g_console.history_flag)
            {
                g_console.history_flag = 0;
                goto RePrint;
            }
            if ((g_console.cursor == g_console.edit_len) && !g_console.delete_flag) /* 追加 */
            {
                PRINTF("%s", temp_buffer);
            }
            else    /* 插入或删除 */
            {
RePrint:
                g_console.delete_flag = 0;
                CURSOR_INVISIBLE();
                CLEAR_LINE();
                CONSOLE_TITLE_LINE(g_console.edit_buffer, g_console.cursor);
                CURSOR_VISIBLE();
            }
            g_console.edit_buffer_changed_flag = 0;
        }
        if (g_console.edit_flag)    /* 提交输入 */
        {
            if (g_console.submit_len > 0) /* 提交长度大于0 */
            {
                memcpy(g_console.history_buffer, g_console.submit_buffer, g_console.submit_len + 1);
                g_console.history_len = g_console.submit_len;
            }

            /* 调用处理函数 */
            task_ret = task_handler(g_console.submit_buffer, g_console.submit_len);
            if (task_ret == -1)
            {
                PRINTF("command not found, type 'help' to get more info\r\n");
            }
            g_console.edit_flag = 0;
            CLEAR_BUFFER(g_console.edit_buffer);
            g_console.edit_len = 0;
            CLEAR_BUFFER(g_console.temp_edit_buffer);
            g_console.temp_edit_len = 0;
            CONSOLE_TITLE();
        }
    }
}

/*** 
 * @brief 串口中断函数，函数名是由宏定义替换，保证可移植性
 * @return [void]
 */
void CONSOLE_UART_IRQ_HANDLER(void)
{
    char rc;

    if (get_console_uart_it_flag(CONSOLE_IT_RXEN) != 0)
    {
        rc = get_console_uart_data();
        g_console.edit_frame[g_console.edit_frame_len++] = rc;
        g_console.edit_frame[g_console.edit_frame_len] = '\0';
    }

    if (get_console_uart_it_flag(CONSOLE_IT_IDLE) != 0) /* idle中断触发，完成一帧数据帧接收 */
    {
        g_console.edit_frame_flag = 1;
        
        clear_console_uart_idel_it_flag();
    }
}

/*** 
 * @brief 模板打印，用法与printf一致，但是不依赖标准输出
 * @param fmt [char*]    
 * @return [void]
 */
void console_printf(const char* fmt, ...)
{
    uint16_t i = 0;
    char *buffer = (char *)malloc(strlen(fmt) + 128);

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    /* 互斥访问串口 */
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    while (buffer[i] != '\0')
    {
        console_uart_send_data(buffer[i]);
        i++;
    }
    xSemaphoreGive(uart_mutex);

    free(buffer);
}
