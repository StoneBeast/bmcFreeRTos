/*
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 17:27:16
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-18 18:14:30
 * @Description  : ipmi相关配置
 */

#ifndef __IPMI_H
#define __IPMI_H

#include "ipmiConfig.h"

#define REQUEST_FORMAT_LEN      7
#define RESPONSE_FORMAT_LEN     8
#define RESPONSE_DATA_START     6
#define IPMI_BMC_LUN            0x00
#define IPMI_BMC_ADDR           0x20
#define VPX_BASE_HARDWARE_ADDR  0x40
#define VPX_HARDWARE_ADDR(GA)   ((unsigned char)(VPX_BASE_HARDWARE_ADDR + GA))
#define VPX_IPMB_ADDR(ha)       ((unsigned char)(ha<<1))
#define WAIT_RESPONSE_MAX       50

#define RES_DATA_LEN(msg_len)   ((uint16_t)(msg_len - RESPONSE_FORMAT_LEN))
#define ASSERENT_VPX_HARDWARE_ADDR(addr)  ((addr >= 0x41) && (addr <= 0x5F))

#define FILL_IN_MSG_TYPE(msg, NetFn_CMD, rs, rq, data, data_len) \
    msg.netfn = NetFn(NetFn_CMD); \
    msg.rs_lun = IPMI_BMC_LUN; \
    msg.rq_seq = generic_rqseq(); \
    msg.rq_lun = IPMI_BMC_LUN; \
    msg.rs_sa = rs; \
    msg.rq_sa = rq; \
    msg.cmd = CMD(NetFn_CMD); \
    memcpy(msg.data, data, data_len); \
    msg.data_len = data_len;

#endif // !__IPMI_H
