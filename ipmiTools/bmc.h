/*
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 15:49:44
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-07 15:42:58
 * @Description  : 向外暴露的bmc接口
 */

#ifndef __BMC_H
#define __BMC_H

#include "ipmiConfig.h"
#include "ipmiMessage.h"
#include "ipmiCommand.h"

#ifdef IS_BMC
uint8_t init_bmc(void);
uint16_t get_ipmc_status(void);
#else
uint8_t init_ipmc(void);
#endif // !IS_BMC

uint8_t ipmi_request(uint8_t rs_sa, uint16_t NetFn_CMD, uint8_t* data, uint16_t data_len);
uint8_t ipmi_response(uint8_t rq_sa, uint16_t NetFn_CMD, uint8_t cmpl_code, uint8_t* data, uint16_t data_len);


#endif //! __BMC_H



