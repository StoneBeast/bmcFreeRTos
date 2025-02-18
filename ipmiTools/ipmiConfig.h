#ifndef __IPMI_CONFIG_H
#define __IPMI_CONFIG_H

#define IS_BMC

#ifndef IS_BMC
#define IS_IPMC
#endif // DEBUG

#define SLOT_COUNT 8

#endif // !__IPMI_CONFIG_H
