#include "nct75.h"
#include "nct75Port.h"
#include <string.h>
#include <stdlib.h>

// NCT75 Register map
#define NCT75_REG_TEMP   0x00
#define NCT75_REG_CONFIG 0x01
#define NCT75_REG_THYST  0x02
#define NCT75_REG_TOS    0x03

// Configuration bits
#define NCT75_CONFIG_SD  (1 << 0) // Shutdown mode
#define NCT75_CONFIG_TM  (1 << 1) // Thermostat mode
#define NCT75_CONFIG_POL (1 << 2) // OS output polarity
#define NCT75_CONFIG_F0  (1 << 3) // Fault queue F0
#define NCT75_CONFIG_F1  (1 << 4) // Fault queue F1
#define NCT75_CONFIG_R0  (1 << 5) // Resolution bit0
#define NCT75_CONFIG_R1  (1 << 6) // Resolution bit1
#define NCT75_CONFIG_OS  (1 << 7) // One-shot conversion

uint8_t NCT75_SetResolution(const nct75_t *p_nct75, NCT75_Resolution res)
{
    uint8_t config;
    HAL_StatusTypeDef ret;
    // Read current config
    ret = HAL_I2C_Mem_Read(&NCT75_I2C, p_nct75->slave_addr, NCT75_REG_CONFIG, I2C_MEMADD_SIZE_8BIT, &config, 1, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;
    // Mask out resolution bits
    config &= ~(NCT75_CONFIG_R0 | NCT75_CONFIG_R1);
    // Set new resolution
    config |= (uint8_t)res;
    // Write back
    return HAL_I2C_Mem_Write(&NCT75_I2C, p_nct75->slave_addr, NCT75_REG_CONFIG, I2C_MEMADD_SIZE_8BIT, &config, 1, HAL_MAX_DELAY);
}

void _init_nct75(nct75_t *p_nct75, uint8_t addr, NCT75_Resolution resolution)
{

    /* TODO: 配置项合法性验证 */

    p_nct75->slave_addr = (uint8_t)((addr<<1)&0xFE);
    p_nct75->resolution = resolution;
    NCT75_SetResolution(p_nct75, resolution);
}


/*** 
 * @brief 读取温度数据，!!若使用one-shot模式，则不能在中断中读取!!
 * @param p_nct75 [nct75_t*]    nct75实例
 * @return [short]           读取数据
 */
short nct75_read_rawData(const nct75_t* p_nct75)
{
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    // Read two bytes from temperature register
    ret = HAL_I2C_Mem_Read(&NCT75_I2C, p_nct75->slave_addr, NCT75_REG_TEMP, I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;

    // Combine bytes: MSB is temperature sign and integer bits, LSB holds fractional bits
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    // Shift right by 4 (12-bit resolution)
    raw >>= 4;

    return raw;
}

/***
 * @brief 将 nct75_read_rawData() 获取的原始数据转换为实际值
 * @param rawData [short]   raw data
 * @return [float]          实际数据
 */
float nct75_temperature_data_conversion(short rawData)
{
    float ret_data;
    ret_data = (float)(rawData*0x0625);

    return ret_data;
}
