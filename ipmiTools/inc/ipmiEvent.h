#ifndef __IPMI_EVENT_H
#define __IPMI_EVENT_H

#include <stdint.h>

#define EVENT_MAX_COUNT                 15

#define EVENT_BODY_ADDR_OFFSET          0
#define EVENT_BODY_SENSOR_NO_OFFSET     1
#define EVENT_BODY_MIN_VAL_OFFSET       2
#define EVENT_BODY_READ_VAL_OFFSET      3
#define EVENT_BODY_MAX_VAL_OFFSET       4
#define EVENT_BODY_UNIT_CODE_OFFSET     5
#define EVENT_BODY_START_M_OFFSET       6
#define EVENT_BODY_START_M_LEN          7
#define EVENT_BODY_NAME_LEN_OFFSET      12
#define EVENT_BODY_NAME_OFFSET          13

typedef struct {
    uint8_t addr;
    uint8_t sensor_no;
    uint8_t sensor_name_len;
    char sensor_name[16];
    uint16_t min_val;
    uint16_t read_val;
    uint16_t max_val;
    short M;
    short K2;
    uint8_t unit_code;
}sensor_ev_t;

#endif // !__IPMI_EVENT_H
