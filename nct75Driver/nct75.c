#include "nct75.h"
#include "nct75Port.h"
#include <string.h>
#include <stdlib.h>

#define REG_STORE_VALUE_ADDR        0x00
#define REG_CONFIG_ADDR             0x01
#define REG_THYST_ADDR              0x02
#define REG_TOS_ADDR                0x03
#define REG_ONE_SHOT_ADDR           0x04
#define ASSERT_REGISTER(reg)       ((reg == REG_STORE_VALUE_ADDR)   ||  \
                                    (reg == REG_CONFIG_ADDR)        ||  \
                                    (reg == REG_THYST_ADDR)         ||  \
                                    (reg == REG_TOS_ADDR)           ||  \
                                    (reg == REG_ONE_SHOT_ADDR))

#define CONFIG_MUSK_SHUTDOWN        0x01
#define CONFIG_MUSK_ALERT_MODE      0x02
#define CONFIG_MUSK_ALERT_PIN_POL   0x04
#define CONFIG_MUSK_FAULT_QUEUE     0x0C
#define CONFIG_MUSK_ONESHOT_MODE    0x10

static void _nct75_write_reg(const nct75_t* p_nct75, uint8_t reg_addr, const uint8_t* wdata, uint16_t data_len)
{
    uint8_t* w_raw = malloc(data_len+1);
    w_raw[0] = reg_addr;
    memcpy(w_raw+1, wdata, data_len);

    NCT75_I2C_WRITE(p_nct75->slave_addr, w_raw, data_len+1);
}

static void _nct75_read_reg(const nct75_t* p_nct75, uint8_t reg_addr, uint8_t* rdata, uint16_t data_len)
{
    NCT75_I2C_WRITE(p_nct75->slave_addr, &reg_addr, 1);
    NCT75_I2C_READ(p_nct75->slave_addr, rdata, data_len);
}

static void _nct75_set_hyst_os(const nct75_t* p_nct75, uint16_t hyst, uint16_t os)
{
    _nct75_write_reg(p_nct75, REG_THYST_ADDR, (uint8_t*)(&hyst), 2);
    _nct75_write_reg(p_nct75, REG_TOS_ADDR, (uint8_t*)(&os), 2);
}

static void _nct75_set_config(const nct75_t* p_nct75, const nct75Conig_t* p_cfg)
{
    uint8_t cfg_byte = 0;

    cfg_byte |= ((p_cfg->shutdown_mode << 0) & CONFIG_MUSK_SHUTDOWN);
    cfg_byte |= ((p_cfg->alert_mode << 1) & CONFIG_MUSK_ALERT_MODE);
    cfg_byte |= ((p_cfg->alert_polarity << 2) & CONFIG_MUSK_ALERT_PIN_POL);
    cfg_byte |= ((p_cfg->fault_queue << 3) & CONFIG_MUSK_FAULT_QUEUE);
    cfg_byte |= ((p_cfg->one_shot_mode << 5) & CONFIG_MUSK_ONESHOT_MODE);

    _nct75_write_reg(p_nct75, REG_CONFIG_ADDR, &cfg_byte, 1);
}

static void _nct75_switch(const nct75_t* p_nct75, uint8_t is_down)
{
    uint8_t cfg_byte = 0;

    if(p_nct75->shutdown_mode) {
        _nct75_read_reg(p_nct75, REG_CONFIG_ADDR, &cfg_byte, 1);
        cfg_byte &= 0xFE;
        cfg_byte |= ((p_nct75->shutdown_mode << 0) & CONFIG_MUSK_SHUTDOWN);
    }
}

void _init_nct75(nct75_t *p_nct75, uint8_t addr, uint8_t is_one_shot, const nct75Conig_t* config)
{

    /* TODO: 配置项合法性验证 */

    p_nct75->slave_addr = (uint8_t)((addr<<1)&0xFE);
    p_nct75->one_shot_mode = 0;
    p_nct75->shutdown_mode = 0;

    nct75Conig_t temp_cfg  = {
        .alert_mode     = 0,
        .alert_polarity = 0,
        .fault_queue    = 0,
        .hysteresis     = 0,
        .one_shot_mode  = 0,
        .overtemperture = 0,
        .shutdown_mode  = 0,
    };

    if (config != NULL) {
        p_nct75->one_shot_mode = config->one_shot_mode;
        p_nct75->shutdown_mode = config->shutdown_mode;
        memcpy(&temp_cfg, config, sizeof(nct75Conig_t));
        _nct75_set_hyst_os(p_nct75, temp_cfg.hysteresis, temp_cfg.overtemperture);
    } else {
        p_nct75->one_shot_mode = is_one_shot;
    }

    _nct75_set_config(p_nct75, &temp_cfg);
}


/*** 
 * @brief 读取温度数据，!!若使用one-shot模式，则不能在中断中读取!!
 * @param p_nct75 [nct75_t*]    nct75实例
 * @return [short]           读取数据
 */
short nct75_read_rawData(const nct75_t* p_nct75)
{
    uint8_t temp_data = 1;
    uint16_t temperature_data;
    uint16_t temp_t_data;

    if (p_nct75->shutdown_mode)
        _nct75_switch(p_nct75, 0);

    if (p_nct75->one_shot_mode) {
        _nct75_write_reg(p_nct75, REG_ONE_SHOT_ADDR, &temp_data, 1);
        NCT75_Delay(50);
    }

    _nct75_read_reg(p_nct75, REG_STORE_VALUE_ADDR, (uint8_t*)(&temperature_data), 2);
    temp_t_data = temperature_data;
    temp_t_data &= 0x8000;

    temperature_data >>= 4;
    temperature_data |= temp_t_data;

    if (p_nct75->shutdown_mode)
        _nct75_switch(p_nct75, 1);

    return ((short)(temperature_data));
}

/***
 * @brief 将 nct75_read_rawData() 获取的原始数据转换为实际值
 * @param rawData [short]   raw data
 * @return [float]          实际数据
 */
float nct75_temperature_data_conversion(short rawData)
{
    float ret_data;
    ret_data = (float)(rawData/16);

    return ret_data;
}
