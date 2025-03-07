/*
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 15:54:30
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-07 17:12:05
 * @Description  : ipmi消息结构定义
 */

#ifndef __IPMI_MESSAGE_H
#define __IPMI_MESSAGE_H

#include <stdint.h>

#define MSG_MAX_LEN     64
#define RES_MSG_MAX_LEN 72

typedef struct ipmi_msg_t
{
    uint8_t netfn;    /* 6bit: [2:7] */
    uint8_t rs_lun;   /* 2bit: [0:1] */
    uint8_t rq_seq;   /* 6bit: [2:7] */
    uint8_t rq_lun;   /* 2bit: [0:1] */
    uint8_t rs_sa;    /* 7bit: [1:7] + 0b */
    uint8_t rq_sa;    /* 7bit: [1:7] + 0b */
    uint8_t cmd;
    uint8_t data[MSG_MAX_LEN];
    uint16_t data_len;
    uint32_t msg_ticks;
} ipmi_req_t;

typedef struct 
{
    struct ipmi_msg_t msg;
    uint8_t comp_code;
}ipmi_resp_t;

typedef struct 
{
    uint8_t msg[RES_MSG_MAX_LEN];
    uint16_t msg_len;
}ipmb_recv_t;

#endif // !__IPMI_MESSAGE_H
