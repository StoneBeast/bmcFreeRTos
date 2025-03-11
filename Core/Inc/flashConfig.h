/*
 * @Author       : stoneBeast
 * @Date         : 2025-03-10 16:07:31
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-11 16:44:45
 * @Description  : flash的相关参数
 */

#ifndef __FLASH_CONFIG_H
#define __FLASH_CONFIG_H

#define W25Q32_PAGE_SIZE          0x100
#define W25Q32_SECTOR_SIZE        0x1000
#define W25Q32_SECTOR_MAX_COUNT   0x0400
#define W25Q32_SECTOR_COUNT       W25Q32_SECTOR_MAX_COUNT
#define W25Q32_START_ADDRESS      0x0000
#endif // !__FLASH_CONFIG_H
