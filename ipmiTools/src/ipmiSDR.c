/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-05 18:52:48
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-19 18:14:22
 * @Description  : SDR相关操作函数
 */

#include "ipmiSDR.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "ipmiHardware.h"
#include "ipmiEvent.h"
#include "link_list.h"
#include "cmsis_os.h"
#include "myPorth.h"

extern SemaphoreHandle_t event_list_mutex;

/*** 
 * @brief 返回单位
 * @param unit_code [uint8_t]   单位代码
 * @return [char*]              [free]单位字符串
 */
static char* get_unit(uint8_t unit_code)
{
    char* ret_unit;
    ret_unit = MALLOC(3);
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
        FREE(ret_unit);
        ret_unit = NULL;
        break;
    }

    return ret_unit;
}

/*** 
 * @brief 从2byte字节的数据中获取M、B等参数
 * @param byte_L [uint8_t]  低位字节
 * @param byte_H [uint8_t]  高位字节
 * @return [short]          目标值    
 */
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

float data_conversion(short data, uint8_t* sdr_start_M)
{
    short M, B, k1, k2;
    uint8_t temp_k;
    float ret_data;

    M = get_M_B(sdr_start_M[0], sdr_start_M[1]);
    B = get_M_B(sdr_start_M[2], sdr_start_M[3]);

    /* 提取k1\k2参数 */
    temp_k = (sdr_start_M[5] & 0x0F);
    if (temp_k & 0x08) {
        /* 负数 */
        k1 = (short)(temp_k | 0xFFF0);
    } else {
        k1 = (short)temp_k;
    }

    temp_k = ((sdr_start_M[5]>>4) & 0x0F);
    if (temp_k & 0x08) {
        /* 负数 */
        k2 = (short)(temp_k | 0xFFF0);
    } else {
        k2 = (short)temp_k;
    }

    ret_data = ((data*M)+(B*powf(10, k1))) * powf(10, k2);

    return ret_data;
}

/*** 
 * @brief 转换数据
 * @param sdr_start_units1 [uint8_t*]   从unit1字节开始的type01的sdr数据
 * @return [float]                      转换后的数据
 */
float reading_date_conversion(uint8_t* sdr_start_units1)
{
    short raw_value;
    float data = 0;
    
    /* 这里省略判断unit1中关于data的位的标志位，默认为无符号 */
    raw_value = (short)(sdr_start_units1[11]);

    data = data_conversion(raw_value, &(sdr_start_units1[4]));

    return data;
}

/*** 
 * @brief 获取将转换后的数据与单位组合后的字符串
 * @param sdr_start_units1 [uint8_t*]   从unit1字节开始的type01的sdr数据
 * @return [char*]                      [free]指向数据的指针
 */
char* get_val_str(uint8_t* sdr_start_units1)
{
    char* ret_str = MALLOC(10);

    float data = reading_date_conversion(sdr_start_units1);
    char *unit = get_unit(sdr_start_units1[1]);

    sprintf(ret_str, "%.3f %s", data, unit);
    FREE(unit);

    return ret_str;
}

uint8_t index_sdr(sdr_index_info_t* sdr_info)
{
    uint16_t start_addr = 0x0000;
    uint16_t temp_id;
    uint8_t temp_len;
    uint8_t temp_sensor_num;
    uint8_t sdr_head[SDR_HEADER_LEN+SDR_KEY_LEN];

    sdr_info->sdr_count = 0;

    while (read_flash(start_addr, SDR_HEADER_LEN, sdr_head))
    {
        temp_id = ((uint16_t*)(sdr_head))[SDR_RECORD_ID_OFFSET];
        temp_len = sdr_head[SDR_RECORD_LEN_OFFSET] + SDR_HEADER_LEN;
        temp_sensor_num = sdr_head[SDR_SENSOR_NUMBER_OFFSET];

        if (temp_id == 0x00 || temp_id > 0x0A)
            return 1;

        (sdr_info->info[sdr_info->sdr_count]).addr = start_addr;
        (sdr_info->info[sdr_info->sdr_count]).id = temp_id;
        (sdr_info->info[sdr_info->sdr_count]).sensor_num = temp_sensor_num;
        
        (sdr_info->info[sdr_info->sdr_count]).len = temp_len;
        sdr_info->sdr_count++;

        /* 更换为at26c16,则需按照16对齐 */
        start_addr += temp_len;
        start_addr = ((start_addr+15)&(~(0x000F)));

    }
    return 0;
}
