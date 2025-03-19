#ifndef __IPMI_EVENT_H
#define __IPMI_EVENT_H

#include <stdint.h>

typedef struct {
    uint8_t addr;
    uint8_t sensor_no;
    char sensor_name[16];
    float min_val;
    float read_val;
    float max_val;
}sensor_ev_t;

#endif // !__IPMI_EVENT_H
