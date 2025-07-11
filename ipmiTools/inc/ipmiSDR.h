/*
 * @Author       : stoneBeast
 * @Date         : 2025-03-07 18:14:30
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-07-10 11:02:35
 * @Description  : 
 */
#ifndef __IPMI_SDR_H
#define __IPMI_SDR_H

#include <stdint.h>
#include "ipmiConfig.h"

/* SDR Header */
#define SDR_HEADER_OFFSET                   0x00
#define SDR_RECORD_ID_OFFSET                0x00
#define SDR_RECORD_ID_LEN                   0x02
#define SDR_SDR_VERSION_OFFSET              0X02
#define SDR_RECORD_TYPE_OFFSET              0x03
#define SDR_RECORD_TYPE_LEN                 0x01
#define SDR_RECORD_LEN_OFFSET	            0x04
#define SDR_RECORD_LEN_LEN	                0x01
#define SDR_HEADER_LEN                      0x05

/* SDR Key Byte */
#define SDR_KEY_OFFSET                      0x05
#define SDR_SENSOR_OWNER_ID_OFFSET          0x05
#define SDR_SENSOR_OWNER_ID_LEN             0x01
#define SDR_OWNER_LUN_OFFSET	            0x06
#define SDR_OWNER_LUN_LEN	                0x01
#define SDR_SENSOR_NUMBER_OFFSET	        0x07
#define SDR_SENSOR_NUMBER_LEN	            0x01
#define SDR_KEY_LEN                         0X03

/* SDR Body */
#define SDR_RECORD_BODY_OFFSET	            0x08
#define SDR_ENTITY_ID_OFFSET	            0x08
#define SDR_ENTITY_ID_LEN	                0x01
#define SDR_ENTITY_INSTANCE_OFFSET	        0x09
#define SDR_ENTITY_INSTANCE_LEN	            0x01
#define SDR_SENSORINITIALIZATION_OFFSET	    0x0A
#define SDR_SENSORINITIALIZATION_LEN	    0x01
#define SDR_SENSOR_CAPABILITIES_OFFSET	    0x0B
#define SDR_SENSOR_CAPABILITIES_LEN	        0x01
#define SDR_SENSOR_TYPE_OFFSET	            0x0C
#define SDR_SENSOR_TYPE_LEN	                0x01
#define SDR_EVENT_READING_TYPE_CODE_OFFSET  0x0D
#define SDR_EVENT_READING_TYPE_CODE_LEN	    0x01
#define SDR_AE_LTR_MASK_OFFSET	            0x0E
#define SDR_AE_LTR_MASK_LEN	                0x02
#define SDR_DE_UTR_MASK_OFFSET	            0x10
#define SDR_DE_UTR_MASK_LEN	                0x02
#define SDR_DR_ST_RT_MASK_OFFSET	        0x12
#define SDR_DR_ST_RT_MASK_LEN	            0x02
#define SDR_SENSOR_UNITS_1_OFFSET	        0x14
#define SDR_SENSOR_UNITS_1_LEN	            0x01
#define SDR_SENSOR_UNITS_2_OFFSET	        0x15
#define SDR_SENSOR_UNITS_2_LEN	            0x01
#define SDR_SENSOR_UNITS_3_OFFSET	        0x16
#define SDR_SENSOR_UNITS_3_LEN	            0x01
#define SDR_LINEARIZATION_OFFSET	        0x17
#define SDR_LINEARIZATION_LEN	            0x01
#define SDR_M_OFFSET	                    0x18
#define SDR_M_LEN	                        0x01
#define SDR_M_TOLERANCE_OFFSET	            0x19
#define SDR_M_TOLERANCE_LEN	                0x01
#define SDR_B_OFFSET	                    0x1A
#define SDR_B_LEN	                        0x01
#define SDR_B_ACCURACY_OFFSET	            0x1B
#define SDR_B_ACCURACY_LEN	                0x01
#define SDR_ACCURACY_EXP_OFFSET	            0x1C
#define SDR_ACCURACY_EXP_LEN	            0x01
#define SDR_R_B_EXP_OFFSET	                0x1D
#define SDR_R_B_EXP_LEN	                    0x01
#define SDR_ANALOG_CHAR_FLAG_OFFSET	        0x1E
#define SDR_ANALOG_CHAR_FLAG_LEN	        0x01
#define SDR_NORMAL_READING_OFFSET	        0x1F
#define SDR_NORMAL_READING_LEN	            0x01
#define SDR_NORMAL_MAX_READING_OFFSET	    0x20
#define SDR_NORMAL_MAX_READING_LEN	        0x01
#define SDR_NORMAL_MIN_READING_OFFSET	    0x21
#define SDR_NORMAL_MIN_READING_LEN	        0x01
#define SDR_SENSOR_MAX_READING_OFFSET	    0x22
#define SDR_SENSOR_MAX_READING_LEN	        0x01
#define SDR_SENSOR_MIN_READING_OFFSET	    0x23
#define SDR_SENSOR_MIN_READING_LEN	        0x01
#define SDR_UNRT_OFFSET	                    0x24
#define SDR_UNRT_LEN	                    0x01
#define SDR_UCT_OFFSET	                    0x25
#define SDR_UCT_LEN	                        0x01
#define SDR_UNCT_OFFSET	                    0x26
#define SDR_UNCT_LEN	                    0x01

/*
    为了保证精度，将这些数据分别修改为Nominal Reading, Maximun, 和Minimumd高字节。
    将数据提升为16位，同时也要根据SDR_SENSOR_UNITS_1位判断是否为有符号数。
*/
#if 0
#define SDR_LNRT_OFFSET	                    0x27
#define SDR_LNRT_LEN	                    0x01
#define SDR_LCT_OFFSET	                    0x28
#define SDR_LCT_LEN	                        0x01
#define SDR_LNCT_OFFSET	                    0x29
#define SDR_LNCT_LEN	                    0x01
#else
#define SDR_NORMAL_READING_HIGH_OFFSET     0X27
#define SDR_NORMAL_READING_HIGH_LEN        0X01
#define SDR_NORMAL_MAXIMUM_HIGH_OFFSET     0X28
#define SDR_NORMAL_MAXIMUM_HIGH_LEN        0X01
#define SDR_NORMAL_MINIMUM_HIGH_OFFSET     0X29
#define SDR_NORMAL_MINIMUM_HIGH_LEN        0X01
#endif //!0

#define SDR_PGTH_VAL_OFFSET	                0x2A
#define SDR_PGTH_VAL_LEN	                0x01
#define SDR_NGTH_VAL_OFFSET	                0x2B
#define SDR_NGTH_VAL_LEN	                0x01
#define SDR_OEM_OFFSET	                    0x2E
#define SDR_OEM_LEN	                        0x01
#define SDR_ID_SRT_TYPE_LEN_OFFSET	        0x2F
#define SDR_ID_SRT_TYPE_LEN_LEN	            0x01
#define SDR_ID_STR_BYTE_OFFSET	            0x30

#define GET_SDR_LEN(p_sdr)                  ((unsigned char)(p_sdr[SDR_RECORD_LEN_OFFSET] + SDR_HEADER_LEN))

#define GET_SDR_INFO_RES_LEN                6
#define GET_SDR_REQ_LEN                     6

#define SDR_MAX_LEN                         4*16
#define SDR_VERIFY                          0x7FF
#define SENSOR_TYPE_TEMPERATURE             0x01
#define SENSOR_TYPE_VOLTAGE                 0x02
#define SENSOR_TYPE_POWER                   0x08
#define SENSOR_UNIT_CODE_DC                 0x01
#define SENSOR_UNIT_CODE_V                  0x04
#define SENSOR_UNIT_CODE_A                  0x05

#define GENGRATE_SDR_DATA(sdr_buf, id, type, units_code, M, MT, R_B, max, min, id_str)                             \
    {                                                                                                              \
        memset(sdr_buf, 0, SDR_MAX_LEN);                                                                           \
        sdr_buf[0]                              = (id & 0x00FF);                                                   \
        sdr_buf[1]                              = ((id >> 8) & 0x00FF);                                            \
        sdr_buf[SDR_ID_SRT_TYPE_LEN_OFFSET]     = strlen(id_str);                                                  \
        sdr_buf[SDR_RECORD_LEN_OFFSET]          = (3 * 16 + sdr_buf[SDR_ID_SRT_TYPE_LEN_OFFSET] - SDR_HEADER_LEN); \
        sdr_buf[SDR_KEY_OFFSET]                 = g_local_addr;                                                    \
        sdr_buf[SDR_SENSOR_NUMBER_OFFSET]       = id;                                                              \
        sdr_buf[SDR_ENTITY_INSTANCE_OFFSET]     = 0x60 + id;                                                       \
        sdr_buf[SDR_SENSOR_TYPE_OFFSET]         = type;                                                            \
        sdr_buf[SDR_SENSOR_UNITS_2_OFFSET]      = units_code;                                                      \
        sdr_buf[SDR_M_OFFSET]                   = M;                                                               \
        sdr_buf[SDR_M_TOLERANCE_OFFSET]         = MT;                                                              \
        sdr_buf[SDR_R_B_EXP_OFFSET]             = R_B;                                                             \
        sdr_buf[SDR_NORMAL_MAX_READING_OFFSET]  = (max & 0x00FF);                                                  \
        sdr_buf[SDR_NORMAL_MAXIMUM_HIGH_OFFSET] = ((max & 0xFF00) >> 8);                                           \
        sdr_buf[SDR_NORMAL_MIN_READING_OFFSET]  = (min & 0x00FF);                                                  \
        sdr_buf[SDR_NORMAL_MINIMUM_HIGH_OFFSET] = ((min & 0xFF00) >> 8);                                           \
        memcpy((sdr_buf + SDR_ID_STR_BYTE_OFFSET), id_str, sdr_buf[SDR_ID_SRT_TYPE_LEN_OFFSET]);                   \
}

typedef struct {
    unsigned short id;
    unsigned short addr;
    unsigned char len;
    unsigned char sensor_num;
}sdr_index_t;

typedef struct{
    sdr_index_t info[SDR_MAX_COUNT];
    unsigned char sdr_count;
}sdr_index_info_t;

typedef struct{
    uint8_t dev_addr;
    uint8_t sdr_id;
    uint8_t sensor_type;
    uint8_t data_unit_code;
    uint8_t is_data_signed;
    uint16_t read_data;
    uint16_t higher_threshold;
    uint16_t lower_threshold;
    uint16_t (*sensor_read)(void);
    short argM;
    short argK2;
    char sensor_name[SENSOR_NAME_MAX_LEN];
    uint8_t name_len;
}Sdr_t;

typedef struct {
    uint8_t id;
    Sdr_t sdr;
}Res_sdr_t;

typedef struct {
    uint8_t sdr_count;
    Sdr_t * p_sdr_list[SDR_MAX_COUNT];
}Sdr_index_t;


unsigned char index_sdr(sdr_index_info_t* sdr_info);
float reading_date_conversion(unsigned char* sdr_start_units1);
char* get_val_str(unsigned char* sdr_start_units1);
float data_conversion(short data, unsigned char* sdr_start_M);
void get_M_K2(short *M, short *K2, unsigned char *sdr_start_units1);

void init_sdr(Sdr_index_t *sdr_index);
uint8_t sdr_setData(Sdr_index_t *sdr, uint8_t sdr_id, uint16_t data);

#endif // !__IPMI_SDR_H
