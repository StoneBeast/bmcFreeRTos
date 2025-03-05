#ifndef __IPMI_SDR_H
#define __IPMI_SDR_H

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
#define SDR_LNRT_OFFSET	                    0x27
#define SDR_LNRT_LEN	                    0x01
#define SDR_LCT_OFFSET	                    0x28
#define SDR_LCT_LEN	                        0x01
#define SDR_LNCT_OFFSET	                    0x29
#define SDR_LNCT_LEN	                    0x01
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

#endif // !__IPMI_SDR_H
