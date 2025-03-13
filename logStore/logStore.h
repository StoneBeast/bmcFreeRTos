#ifndef __LOG_STORE_H
#define __LOG_STORE_H

#define SPI_CS_GPIO_Port    GPIOB
#define SPI_CS_Pin          GPIO_PIN_12

#if 0
#define RX_BUFFER_MAX_LEN       2560
#else
//TODO: 这里测试使用较短的buffer
#define RX_BUFFER_MAX_LEN       128
#endif
extern uint8_t rx_buf_0[RX_BUFFER_MAX_LEN];
extern uint16_t rx_buf_0_len;
extern uint8_t rx_buf_1[RX_BUFFER_MAX_LEN];
extern uint16_t rx_buf_1_len;
extern uint8_t current_buf;

void init_logStore_hardware(void);
uint16_t mount_fs(void);
void register_fs_ops(void);

#endif // !__LOG_STORE_H
