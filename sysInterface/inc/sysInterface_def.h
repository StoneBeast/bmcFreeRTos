/*
 * @Author       : stoneBeast
 * @Date         : 2025-03-28 18:34:45
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-07-11 18:20:40
 * @Description  : 
 */
#ifndef __SYS_INTERFACE_DEF_H
#define __SYS_INTERFACE_DEF_H

#include <stdint.h>

#define __USER
#define REQ_MAX_LEN         128
#define BUFFER_LEN  256

#define MSG_TYPE_OFFSET     0
#define MSG_TYPE_LENGTH     1
#define MSG_CODE_OFFSET     1
#define MSG_CODE_LENGTH     1
#define MSG_LEN_OFFSET      2
#define MSG_LEN_LENGTH      2
#define MSG_DATA_OFFSET     4
#define MSG_DATA_LENGTH     1
#define MSG_FORMAT_LENGTH   5
#define MSG_FILE_PKG_FILE_SIZE_OFFSET       MSG_DATA_OFFSET
#define MSG_FILE_PKG_FILE_SIZE_LEN          4
#define MSG_FILE_PKG_PKG_INDEX_OFFSET       (MSG_DATA_OFFSET + MSG_FILE_PKG_FILE_SIZE_LEN)
#define MSG_FILE_PKG_PKG_INDEX_LEN          2
#define MSG_FILE_PKG_PKG_FLAG_OFFSET        (MSG_FILE_PKG_PKG_INDEX_OFFSET + MSG_FILE_PKG_PKG_INDEX_LEN)
#define MSG_FILE_PKG_PKG_FLAG_LEN           1
#define MSG_FILE_PKG_PKG_DATA_LEN_OFFSET    (MSG_FILE_PKG_PKG_FLAG_OFFSET + MSG_FILE_PKG_PKG_FLAG_LEN)
#define MSG_FILE_PKG_PKG_DATA_LEN_LEN       1
#define MSG_FILE_PKG_PKG_DATA_OFFSET        (MSG_FILE_PKG_PKG_DATA_LEN_OFFSET + MSG_FILE_PKG_PKG_DATA_LEN_LEN)

#define SYS_MSG_TYPE_REQ    0x01
#define SYS_MSG_TYPE_RES    0x02
// #define SYS_MSG_TYPE_EVENT  0x03
#define IS_MSG_TYPE(type)   ((type == SYS_MSG_TYPE_REQ) || \
                           (type == SYS_MSG_TYPE_RES) ||

#define SYS_CMD_DEVICE_LIST   0x01
#define SYS_CMD_DEVICE_SENSOR 0x02
#define SYS_FILE_LIST         0x21
#define SYS_FILE_READ         0x22
#define SYS_FILE_REMOVE       0x23
#define SYS_FILE_REFORMAT     0x24
#define SYS_EVENT_SENSOR_OVER 0x81
#define IS_MSG_CODE(code)     ((code == SYS_CMD_DEVICE_SENSOR) || \
                           (code == SYS_CMD_DEVICE_LIST) ||       \
                           (code == SYS_EVENT_SENSOR_OVER) ||     \
                           (code == SYS_FILE_LIST) ||             \
                           (code == SYS_FILE_READ) ||             \
                           (code == SYS_FILE_REMOVE) ||           \
                           (code == SYS_FILE_REFORMAT))

typedef struct
{
    uint8_t request_msg[REQ_MAX_LEN];
    uint16_t request_len;
} sys_req_t;

#endif // !__SYS_INTERFACE_DEF_H
