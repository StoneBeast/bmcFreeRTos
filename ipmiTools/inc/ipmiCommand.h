#ifndef __IPMI_COMMAND_H
#define __IPMI_COMMAND_H

/** @defgroup NetFn_CMD
  * @{
  */

#define CMD_GET_DEVICE_ID               ((unsigned short)0x0601)
#define CMD_SEND_MESSAGE                ((unsigned short)0x0634)
#define CMD_GET_VSO_CAP                 ((unsigned short)0x2C00)
#define CMD_GET_MANDATORY_SENDOR_NUMBER ((unsigned short)0x2C44)

#define CMD(NetFn_CMD)                  ((unsigned char)NetFn_CMD)
#define NetFn(NetFn_CMD)                ((unsigned char)(NetFn_CMD >> 8))
#define NetFn_Res(NetFn_CMD)            ((unsigned char)((NetFn(NetFn_CMD))|0x01))
#define ADD_LUN(a, LUN)                 ((unsigned char)((a<<2) | (LUN&0x03)))
  /**
    * @}
    */

#endif // !__IPMI_COMMAND_H
