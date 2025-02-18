/*
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 17:16:56
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-08 17:24:33
 * @Description  : 声明ipmi相关硬件接口
 */

#ifndef __IPMI_HARDWARE_H
#define __IPMI_HARDWARE_H

#include <stdint.h>

#define __USER_IMPLEMENTATION

// typedef unsigned char uint8_t;
// typedef unsigned short uint16_t;
// typedef unsigned int uint32_t;

unsigned char __USER_IMPLEMENTATION read_GA_Pin(void);
void __USER_IMPLEMENTATION init_ipmb_i2c(uint8_t local_addr);
uint8_t __USER_IMPLEMENTATION send_i2c_msg(uint8_t* msg, uint16_t len);
void __USER_IMPLEMENTATION init_adc(void);
uint32_t __USER_IMPLEMENTATION get_sys_ticks(void);

#endif // !__IPMI_HARDWARE_H
