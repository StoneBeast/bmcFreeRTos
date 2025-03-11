#ifndef __LOG_STORE_H
#define __LOG_STORE_H

#define SPI_CS_GPIO_Port    GPIOB
#define SPI_CS_Pin          GPIO_PIN_8

void init_logStore_hardware(void);
uint8_t mount_fs(void);
void register_fs_ops(void);

#endif // !__LOG_STORE_H
