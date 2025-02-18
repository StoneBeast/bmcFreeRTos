/*
 * @Author       : stoneBeast
 * @Date         : 2025-01-21 16:25:53
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-01-24 14:12:54
 * @Description  : 存放一些终端操作定义
 */

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "link_list.h"

#define BUFFER_LEN              256


#define ASSERENT_KEY(buf, key)  (memcmp(buf, key, strlen(key)) == 0)
#define PRINTF(...)             console_printf(__VA_ARGS__)

#define KEY_UP              "\x1b\x5b\x41"      /* [up] key: 0x1b 0x5b 0x41 */
#define KEY_DOWN            "\x1b\x5b\x42"      /* [down] key: 0x1b 0x5b 0x42 */
#define KEY_RIGHT           "\x1b\x5b\x43"      /* [right] key: 0x1b 0x5b 0x43 */
#define KEY_LEFT            "\x1b\x5b\x44"      /* [left] key: 0x1b 0x5b 0x44 */
#define KEY_ENTER           '\r'                /* [enter] key */
#define KEY_BACKSPACE       '\x7f'              /* [backspace] key: 0x7f */
#define KEY_HOME            "\x1b\x5b\x48"      /* [home] key: 0x1b 0x5b 0x48 */
#define KEY_END             "\x1b\x5b\x46"      /* [end] key: 0x1b 0x5b 0x46 */
#define KEY_DELETE          "\x1b\x5b\x33\x7e"  /* [delete] key: 0x1b 0x5b 0x33 0x7e */

#define CONSOLE_TITLE_TEXT          "root > "
#define CONSOLE_TITLE_COLORED       "\033[0;32;1m%s\033[0m"
#define CONSOLE_TITLE()             PRINTF(CONSOLE_TITLE_COLORED, CONSOLE_TITLE_TEXT)
#define CONSOLE_OUT(...)            CONSOLE_TITLE(); \
                                    PRINTF(__VA_ARGS__)

#define CLEAR_LINE()                PRINTF("\r\033[K")
#define CONSOLE_TITLE_LINE(s,c)     CONSOLE_OUT("%s\033[%dG", s, c+1+strlen(CONSOLE_TITLE_TEXT))
#define CONSOLE_NEW_LINE()          PRINTF("\r\n")

#define CURSOR_LEFT(x)              PRINTF("\033[%dD", x)
#define CURSOR_RIGHT(x)             PRINTF("\033[%dC", x)
#define CURSOR_INVISIBLE()          PRINTF("\033[?25l")
#define CURSOR_VISIBLE()            PRINTF("\033[?25h")
#define CONSOLE_CLEAR()             PRINTF("\033[2J\033[0;0H")

#define CLEAR_BUFFER(buf)           memset(buf, 0, strlen((char *)buf))

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct
{
    uint8_t submit_buffer[BUFFER_LEN];  /* 提交缓存，最终需要处理的串口输入 */
    uint16_t submit_len;                /* 提交缓存长度 */
    uint8_t edit_buffer[BUFFER_LEN];    /* 用于保存提交之前当前输入、修改后的字符 */
    uint16_t edit_len;                  /* 编辑缓存长度 */
    uint8_t edit_flag;                  /* 编辑缓存标志 */
    uint8_t history_buffer[BUFFER_LEN]; /* 历史输入缓存 */
    uint16_t history_len;               /* 历史输入缓存长度 */
    uint8_t history_flag;               /* 历史输入缓存标志 */
    uint8_t temp_edit_buffer[BUFFER_LEN]; /* 临时编辑缓存 */
    uint16_t temp_edit_len;             /* 临时编辑缓存长度 */
    uint16_t cursor;                    /* 光标位置 */
    uint8_t edit_frame[BUFFER_LEN];     /* 当前输入帧 */
    uint16_t edit_frame_len;            /* 当前输入帧长度 */
    uint8_t edit_frame_flag;            /* 当前输入帧标志 */
    uint8_t relog_flag;                 /* 重新输出标志 */
    uint8_t edit_buffer_changed_flag;   /* 编辑缓存改变标志 */
    uint8_t delete_flag;                /* 删除标志 */
}console_struct;

void console_printf(const char *fmt, ...);

extern link_list_manager* g_console_task_list;  /* 全局task链表 */
extern link_list_manager* g_console_bg_task_list;   /* 全局后台task链表 */

#endif // !__CONSOLE_H
