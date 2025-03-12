/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 17:16:38
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-12 14:26:41
 * @Description  : 实现该平台的规定接口的硬件操作
 */

#include "i2c.h"
#include "adc.h"
#include "dma.h"
#include "ipmiHardware.h"
#include "ipmiConfig.h"

extern uint8_t add_msg_list(uint8_t* msg, uint16_t len);

volatile uint8_t adc_complete_flag = 0;
extern unsigned char g_ipmb_msg[64];
extern unsigned short g_ipmb_msg_len;
extern unsigned char g_local_addr;
extern volatile uint8_t g_adc_conv_flag;
uint16_t adc_data[6] = {0};

unsigned char __USER_IMPLEMENTATION read_GA_Pin(void)
{
    return 0;
}

void __USER_IMPLEMENTATION init_ipmb_i2c(uint8_t local_addr)
{
    /* init i2c */
    MX_I2C1_Init(local_addr);
}

uint8_t __USER_IMPLEMENTATION send_i2c_msg(uint8_t* msg, uint16_t len)
{
    /* send i2c msg */

    HAL_I2C_DisableListen_IT(&hi2c1);

    /* 避免busy被错位置高导致总线锁死 */
    if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY) == SET)
    {
        HAL_Delay(5);
        if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY) == SET)
        {
            HAL_I2C_DeInit(&hi2c1);
            HAL_I2C_Init(&hi2c1);
        }
    }

    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, msg[0], msg+1, len-1, 100);
    HAL_I2C_EnableListen_IT(&hi2c1);

    if (ret == HAL_OK)
        return 1;
    else
        return 0;

}

void __USER_IMPLEMENTATION init_adc(void)
{
    /* init sensor */
    MX_DMA_Init();
    MX_ADC1_Init();
}

void __USER_IMPLEMENTATION init_inter_bus(void)
{
    MX_I2C2_Init();
}

uint8_t __USER_IMPLEMENTATION read_flash(uint16_t addr, uint8_t read_len, uint8_t* data)
{
    HAL_StatusTypeDef read_ret;

    /* 避免busy被错位置高导致总线锁死 */
    if (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY) == SET)
    {
        HAL_Delay(2);
        if (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY) == SET)
        {
            HAL_I2C_DeInit(&hi2c2);
            HAL_I2C_Init(&hi2c2);
        }
    }

    read_ret = HAL_I2C_Mem_Read(&hi2c2, FLASH_ADDR_PADDR(addr), addr, 1, data, read_len, 100);
    if (read_ret != HAL_OK)
        return 0;

    return 1;
}

uint8_t __USER_IMPLEMENTATION write_flash(uint16_t addr, uint8_t write_len, uint8_t* data)
{
    HAL_StatusTypeDef write_ret;

    /* 避免busy被错位置高导致总线锁死 */
    if (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY) == SET)
    {
        HAL_Delay(2);
        if (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY) == SET)
        {
            HAL_I2C_DeInit(&hi2c2);
            HAL_I2C_Init(&hi2c2);
        }
    }

    write_ret = HAL_I2C_Mem_Write(&hi2c2, FLASH_ADDR_PADDR(addr), addr, 1, data, write_len, 100);
    if (write_ret != HAL_OK)
        return 0;

    return 1;
}

uint32_t __USER_IMPLEMENTATION get_sys_ticks(void)
{
    return HAL_GetTick();
}

static void __USER_IMPLEMENTATION read_adc()
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_data, 4);
    while (0 == g_adc_conv_flag);
    g_adc_conv_flag = 0;
}

uint8_t __USER_IMPLEMENTATION read_sdr1_sensor_data(void)
{
    read_adc();
    return adc_data[0]>>4;
}

uint8_t __USER_IMPLEMENTATION read_sdr2_sensor_data(void)
{
    return adc_data[1]>>4;
}

uint8_t __USER_IMPLEMENTATION read_sdr3_sensor_data(void)
{
    return adc_data[2]>>4;
}

uint8_t __USER_IMPLEMENTATION read_sdr4_sensor_data(void)
{
    return adc_data[3]>>4;
}

// 侦听完成回调函数（完成一次完整的i2c通信以后会进入该函数）
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    add_msg_list(g_ipmb_msg, g_ipmb_msg_len);
    g_ipmb_msg_len = 0;
    HAL_I2C_EnableListen_IT(hi2c); // slave is ready again
}

// I2C设备地址回调函数（地址匹配上以后会进入该函数）
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if (g_ipmb_msg_len == 0)
    {
        g_ipmb_msg[0] = g_local_addr;
        g_ipmb_msg_len ++;
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, &(g_ipmb_msg[g_ipmb_msg_len]), 1, I2C_NEXT_FRAME);  // 每次第1个数据均为偏移地址
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    g_ipmb_msg_len++;
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, &(g_ipmb_msg[g_ipmb_msg_len]), 1, I2C_NEXT_FRAME);  // 每次第1个数据均为偏移地址
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	if(hi2c->Instance == I2C1)
	{
	    //从机不再复位总线
	    hi2c->Instance->CR1 = 0;   //复位PE
   		hi2c->Instance->CR1 = 1;    //解除复位
	}
}

