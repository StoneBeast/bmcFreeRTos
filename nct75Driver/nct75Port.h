#ifndef __NCT75_PORT_H
#define __NCT75_PORT_H

#include "i2c.h"
#include "cmsis_os.h"

#define NCT75_I2C hi2c2

#define NCT75_I2C_WRITE(addr, p_wdata, wdata_len)   HAL_I2C_Master_Transmit(&NCT75_I2C, addr, \
                                                                            p_wdata, wdata_len, 100)
#define NCT75_I2C_READ(addr, p_rdata, rdata_len)    HAL_I2C_Master_Receive(&NCT75_I2C, addr, \
                                                                            p_rdata, rdata_len, 100)

#define NCT75_Delay(ms)                             vTaskDelay(pdMS_TO_TICKS(ms))                

#endif // !__NCT75_PORT_H
