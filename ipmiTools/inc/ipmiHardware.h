/*
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 17:16:56
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-09 15:03:26
 * @Description  : 声明ipmi相关硬件接口
 */

#ifndef __IPMI_HARDWARE_H
#define __IPMI_HARDWARE_H

#include <stdint.h>

#define __USER_IMPLEMENTATION

unsigned char __USER_IMPLEMENTATION read_GA_Pin(void);
void __USER_IMPLEMENTATION init_ipmb_i2c(uint8_t local_addr);
uint8_t __USER_IMPLEMENTATION send_i2c_msg(uint8_t* msg, uint16_t len);
void __USER_IMPLEMENTATION init_adc(void);
void __USER_IMPLEMENTATION init_inter_bus(void);
uint8_t __USER_IMPLEMENTATION read_flash(uint16_t addr, uint8_t read_len, uint8_t* data);
uint8_t __USER_IMPLEMENTATION write_flash(uint16_t addr, uint8_t write_len, uint8_t* data);
uint32_t __USER_IMPLEMENTATION get_sys_ticks(void);

uint16_t __USER_IMPLEMENTATION read_sdr1_sensor_data(void);
uint16_t __USER_IMPLEMENTATION read_sdr2_sensor_data(void);
uint16_t __USER_IMPLEMENTATION read_sdr3_sensor_data(void);
uint16_t __USER_IMPLEMENTATION read_sdr4_sensor_data(void);

#endif // !__IPMI_HARDWARE_H
