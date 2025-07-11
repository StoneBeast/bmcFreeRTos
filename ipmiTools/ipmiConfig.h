#ifndef __IPMI_CONFIG_H
#define __IPMI_CONFIG_H

#define IS_BMC

#ifndef IS_BMC
#define IS_IPMC
#endif // DEBUG

#define SENSOR_COUNT    6
#define SLOT_COUNT 8
#define SDR_MAX_COUNT   10
#define SENSOR_NAME_MAX_LEN         12
#define AT24C16_PAGE_SIZE 16
#define FLASH_I2C_BASS_ADDR 0x50
#define FLASH_ADDR_PADDR(w_addr)    ((uint8_t)((FLASH_I2C_BASS_ADDR|((w_addr>>8) & 0x0007))<<1))

#define USE_DEBUG_CMD   1
#define DEBUG_CMD_PREFIX    "debug_cmd "
#define DEBUG_CMD_READ_AD   'A'
#endif // !__IPMI_CONFIG_H
