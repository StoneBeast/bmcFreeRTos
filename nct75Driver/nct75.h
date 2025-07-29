/*
 * @Author       : stoneBeast
 * @Date         : 2025-04-14 10:34:15
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-07-10 15:07:34
 * @Description  : nct75驱动api
 */

#ifndef __NCT75_H
#define __NCT75_H

#include <stdint.h>

#define NCT75_ALERT_MODE_COMP           ((uint8_t)0)
#define NCT75_ALERT_MODE_INT            ((uint8_t)1)
#define NCT75_ALERT_PIN_POLARITY_LOW    ((uint8_t)0)
#define NCT75_ALERT_PIN_POLARITY_HIGH   ((uint8_t)1)

// Possible resolutions (bits R1:R0)
typedef enum {
    NCT75_RES_9BIT  = 0x00,
    NCT75_RES_10BIT = 0x20,
    NCT75_RES_11BIT = 0x40,
    NCT75_RES_12BIT = 0x60,
} NCT75_Resolution;
typedef struct 
{
    uint8_t slave_addr;
    NCT75_Resolution resolution;
}nct75_t;

#define simple_init_nct75(p_nct75, addr, resolution)    _init_nct75(p_nct75, addr, resolution)

void _init_nct75(nct75_t *p_nct75, uint8_t addr, NCT75_Resolution resolution);
short nct75_read_rawData(const nct75_t* p_nct75);
float nct75_temperature_data_conversion(short rawData);
uint8_t NCT75_SetResolution(const nct75_t *p_nct75, NCT75_Resolution res);

#endif // !__NCT75_H
 