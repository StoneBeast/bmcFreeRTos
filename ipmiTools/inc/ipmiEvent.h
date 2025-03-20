#ifndef __IPMI_EVENT_H
#define __IPMI_EVENT_H

#include <stdint.h>

#define EVENT_BODY_ADDR_OFFSET          0
#define EVENT_BODY_SENSOR_NO_OFFSET     1
#define EVENT_BODY_MIN_VAL_OFFSET       2
#define EVENT_BODY_READ_VAL_OFFSET      3
#define EVENT_BODY_MAX_VAL_OFFSET       4
#define EVENT_BODY_START_M_OFFSET       5
#define EVENT_BODY_START_M_LEN          6
#define EVENT_BODY_NAME_LEN_OFFSET      11
#define EVENT_BODY_NAME_OFFSET          12

typedef struct {
    uint8_t addr;
    uint8_t sensor_no;
    char sensor_name[16];
    float min_val;
    float read_val;
    float max_val;
}sensor_ev_t;

#endif // !__IPMI_EVENT_H
