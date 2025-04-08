/*
 * @Author       : stoneBeast
 * @Date         : 2025-03-11 18:10:39
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-01 18:52:47
 * @Description  : 存放日志存储功能相关参数设置以及定义等
 */

#ifndef __LOG_STORE_H
#define __LOG_STORE_H

#include "lfs.h"
#include "w25qxx.h"

#define SPI_CS_GPIO_Port    GPIOB
#define SPI_CS_Pin          GPIO_PIN_12

#define READ_BUFFER_MAX_LEN     256
#define RX_BUFFER_MAX_LEN       2560

#define MIN_AVAILABLE_SPACE 1000
extern uint8_t rx_buf_0[RX_BUFFER_MAX_LEN];
extern uint16_t rx_buf_0_len;
extern uint16_t rx_buf_0_last_len;
extern uint8_t rx_buf_1[RX_BUFFER_MAX_LEN];
extern uint16_t rx_buf_1_len;
extern uint16_t rx_buf_1_last_len;
extern uint8_t current_buf;
extern uint8_t fs_flag;
extern uint8_t start_flag;
extern uint8_t end_flag;
extern W25QXX_HandleTypeDef w25qxx;
extern lfs_t lfs;

void init_logStore_hardware(void);
uint16_t mount_fs(void);
// void register_fs_ops(void);
uint16_t get_file_count(void);

#endif // !__LOG_STORE_H
