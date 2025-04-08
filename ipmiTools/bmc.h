/*
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 15:49:44
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-07 13:33:02
 * @Description  : 向外暴露的bmc接口
 */

#ifndef __BMC_H
#define __BMC_H

#include "ipmiConfig.h"
#include "ipmiMessage.h"
#include "ipmiCommand.h"
#include "ipmi.h"
#include "ipmiEvent.h"

#ifdef IS_BMC
uint8_t init_bmc(void);
uint16_t get_ipmc_status(void);
#else
#endif // !IS_BMC

uint8_t ipmi_request(uint8_t rs_sa, uint16_t NetFn_CMD, uint8_t* data, uint16_t data_len);
uint8_t ipmi_response(uint8_t rq_sa, uint16_t NetFn_CMD, uint8_t cmpl_code, uint8_t* data, uint16_t data_len);

fru_t* ipmi_get_device_ID(uint8_t dev_ipmi_addr);
uint8_t *scan_device(uint16_t *device_count);
uint8_t *get_sensor_list(uint8_t ipmi_addr, uint16_t *ret_data_len);
void push_event(sensor_ev_t ev);
uint8_t pop_event(sensor_ev_t *ret);
void push_event_from_isr(sensor_ev_t ev);
uint8_t pop_event_from_isr(sensor_ev_t *ret);
uint8_t get_event_count(void);
uint8_t get_event_count_from_isr(void);

#endif //! __BMC_H



