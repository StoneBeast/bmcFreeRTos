/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-05 18:52:48
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-06 14:53:30
 * @Description  : SDR相关操作函数
 */

#include "ipmiSDR.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

static char* get_unit(uint8_t unit_code)
{
    char* ret_unit;
    ret_unit = malloc(3);
    switch (unit_code)
    {
    case 0x01:
        strcpy(ret_unit, "dC");
        break;
    case 0x04:
        strcpy(ret_unit, "V");
        break;
    case 0x05:
        strcpy(ret_unit, "A");
        break;
    default:
        free(ret_unit);
        ret_unit = NULL;
        break;
    }

    return ret_unit;
}

static short get_M_B(uint8_t byte_L, uint8_t byte_H)
{
    // 1. 提取高2位
    uint8_t high_bits = (byte_H >> 6) & 0x03; 

    // 2. 组合为10位数值
    uint16_t combined = (high_bits << 8) | byte_L; 

    // 3. 符号扩展至16位
    short m_short;
    if (combined & 0x0200) { 
        m_short = (short)(combined | 0xFC00); 
    } else {
        m_short = (short)combined;
    }

    return m_short;
}

float reading_date_conversion(uint8_t* sdr_start_units1)
{
    short M, B, k1, k2;
    uint8_t temp_k;
    short raw_value;
    float data = 0;

    M = get_M_B(sdr_start_units1[4], sdr_start_units1[5]);
    B = get_M_B(sdr_start_units1[6], sdr_start_units1[7]);

    temp_k = (sdr_start_units1[9] & 0x0F);
    if (temp_k & 0x08) {
        /* 负数 */
        k1 = (short)(temp_k | 0xFFF0);
    } else {
        k1 = (short)temp_k;
    }

    temp_k = ((sdr_start_units1[9]>>4) & 0x0F);
    if (temp_k & 0x08) {
        /* 负数 */
        k2 = (short)(temp_k | 0xFFF0);
    } else {
        k2 = (short)temp_k;
    }
    
    /* 这里省略判断unit1中关于data的位的标志位，默认为无符号 */
    raw_value = (short)(sdr_start_units1[11]);

    data = ((raw_value*M)+(B*powf(10, k1))) * powf(10, k2);

    return data;
}

char* get_val_str(uint8_t* sdr_start_units1)
{
    char* ret_str = malloc(10);

    float data = reading_date_conversion(sdr_start_units1);
    char *unit = get_unit(sdr_start_units1[1]);

    sprintf(ret_str, "%.3f %s", data, unit);
    free(unit);

    return ret_str;
}

//TODO: 数据转换，单位匹配
