/*
 * @Author       : stoneBeast
 * @Date         : 2025-02-18 09:40:55
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-21 10:25:55
 * @Description  : ipmi命令相关宏定义
 */

#ifndef __IPMI_COMMAND_H
#define __IPMI_COMMAND_H

/** @defgroup NetFn_CMD
  * @{
  */

#define CMD_GET_DEVICE_ID               ((unsigned short)0x0601)
#define CMD_SEND_MESSAGE                ((unsigned short)0x0634)
#define CMD_GET_VSO_CAP                 ((unsigned short)0x2C00)
#define CMD_GET_MANDATORY_SENDOR_NUMBER ((unsigned short)0x2C44)
#define CMD_GET_DEVICE_SDR_INFO         ((unsigned short)0x0420)
#define CMD_GET_DEVICE_SDR              ((unsigned short)0x0421)
#define CMD_RESERVE_SDR_REPOSITORY      ((unsigned short)0x0422)
#define CMD_GET_SENSOR_READING          ((unsigned short)0x042D)

#define IS_IPMI_CMD(cmd)                ((cmd == CMD_GET_DEVICE_ID)               ||  \
                                         (cmd == CMD_SEND_MESSAGE)                ||  \
                                         (cmd == CMD_GET_VSO_CAP)                 ||  \
                                         (cmd == CMD_GET_MANDATORY_SENDOR_NUMBER) ||  \
                                         (cmd == CMD_GET_DEVICE_SDR_INFO)         ||  \
                                         (cmd == CMD_GET_DEVICE_SDR)              ||  \
                                         (cmd == CMD_RESERVE_SDR_REPOSITORY)      ||  \
                                         (cmd == CMD_GET_SENSOR_READING))

#define CMD(NetFn_CMD)                  ((unsigned char)NetFn_CMD)
#define NetFn(NetFn_CMD)                ((unsigned char)(NetFn_CMD >> 8))
#define NetFn_Res(NetFn_CMD)            ((unsigned char)((NetFn(NetFn_CMD))|0x01))
#define ADD_LUN(a, LUN)                 ((unsigned char)((a<<2) | (LUN&0x03)))
  /**
    * @}
    */

#endif // !__IPMI_COMMAND_H
