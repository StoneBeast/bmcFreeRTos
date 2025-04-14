/*
 * @Author       : stoneBeast
 * @Date         : 2025-04-14 10:34:15
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-14 18:19:48
 * @Description  : nct75驱动api
 */

#ifndef __NCT75_H
#define __NCT75_H

#include <stdint.h>

#define NCT75_ALERT_MODE_COMP           ((uint8_t)0)
#define NCT75_ALERT_MODE_INT            ((uint8_t)1)
#define NCT75_ALERT_PIN_POLARITY_LOW    ((uint8_t)0)
#define NCT75_ALERT_PIN_POLARITY_HIGH   ((uint8_t)1)

typedef struct 
{
    uint8_t slave_addr;
    uint8_t one_shot_mode;
    uint8_t shutdown_mode;
}nct75_t;

typedef struct {
    short hysteresis;
    short overtemperture;
    uint8_t one_shot_mode;
    uint8_t fault_queue;
    uint8_t alert_polarity;
    uint8_t alert_mode;
    uint8_t shutdown_mode;
}nct75Conig_t;

#define simple_init_nct75(p_nct75, addr, is_one_shot)   _init_nct75(p_nct75, addr, is_one_shot, NULL)
#define init_nct75(p_nct75, addr, p_confg)              _init_nct75(p_nct75, addr, 0, p_confg)

void _init_nct75(nct75_t* p_nct75, uint8_t addr, uint8_t is_one_shot, const nct75Conig_t* config);
short nct75_read_rawData(const nct75_t* p_nct75);
float nct75_temperature_data_conversion(short rawData);

#endif // !__NCT75_H
 