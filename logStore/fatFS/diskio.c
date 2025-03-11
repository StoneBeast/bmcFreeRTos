/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "flashConfig.h"
#include "w25qxx.h"
#include "main.h"
#include "logStore.h"

extern W25QXX_HandleTypeDef w25qxx;
extern SPI_HandleTypeDef hspi1;
static volatile DSTATUS Stat = STA_NOINIT;
/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  UNUSED(pdrv);

	if (w25qxx.device_id == 0x4016 || w25qxx.device_id == 0x4018)
  {
    return FR_OK;
  }

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  UNUSED(pdrv);

  if (W25QXX_Ok == w25qxx_init(&w25qxx, &hspi1, SPI_CS_GPIO_Port, SPI_CS_Pin))
  {
    return RES_OK;
  }

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
  UNUSED(pdrv);

  W25QXX_result_t w25_res = w25qxx_read(&w25qxx, sector * w25qxx.sector_size, buff, count * w25qxx.sector_size);
  if (w25_res == W25QXX_Ok)
  {
    return RES_OK;
  }

	return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
  UNUSED(pdrv);

  if (count == W25Q32_SECTOR_COUNT)
  {
    w25qxx_chip_erase(&w25qxx);
  }
  else
  {
    w25qxx_erase(&w25qxx, sector*w25qxx.sector_size, count*w25qxx.sector_size);
  }

  if(W25QXX_Ok == w25qxx_write(&w25qxx, sector * w25qxx.sector_size, (uint8_t*)buff, count * w25qxx.sector_size))
  {
    return RES_OK;
  }
	return RES_ERROR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  /*
   *  CTRL_SYNC			0
      GET_SECTOR_COUNT
      GET_SECTOR_SIZE
      GET_BLOCK_SIZE
      CTRL_TRIM			4
   * */
  UNUSED(pdrv);

  switch (cmd)
  {
    case GET_SECTOR_COUNT:
      *(DWORD *)buff = w25qxx.sectors_in_block * w25qxx.block_count;
      break;
    case GET_SECTOR_SIZE:
      *(DWORD *)buff = w25qxx.sector_size;
      break;
    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1;
      break;
    default:
      return RES_OK;
  }

	return RES_OK;
}

