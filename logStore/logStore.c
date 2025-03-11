/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-10 16:28:57
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-11 18:04:53
 * @Description  : 
 */

#include "spi.h"
#include "w25qxx.h"
#include "flashConfig.h"
#include "ff.h"
#include "logStore.h"
#include "uartConsole.h"
#include <stdlib.h>
#include <string.h>
#include "console.h"

#define FS_CHECK()  if(0 == fs_flag) {                          \
                        PRINTF("file system not mounted\r\n");  \
                        return -1;}                             \

uint8_t fs_flag = 0;
W25QXX_HandleTypeDef w25qxx;
uint8_t work_sp[W25Q32_SECTOR_SIZE];
FATFS fs;
MKFS_PARM opt = {
    .fmt = FM_FAT,
    .n_fat = 0,
    .align = 1,
    .au_size = 4096,
    .n_root =0
};

static int fs_ls_func(int argc, char* argv[]);
static int fs_mk_func(int argc, char* argv[]);
static int fs_w_func(int argc, char* argv[]);
static int fs_cat_func(int argc, char* argv[]);
static int fs_rm_func(int argc, char* argv[]);
static int fs_reformat_func(int argc, char* argv[]);

Task_t fs_opts[] = {
    {"fs_ls", "list file in current directory", fs_ls_func},
    {"fs_mk", "fs_mk <name>, make a new file named 'name'", fs_mk_func},
    {"fs_w", "fs_w <name> <str>, write 'str' to 'name'", fs_w_func}, /* 调试使用 */
    {"fs_cat", "fs_cat <name> [count], display file [count]/full byte", fs_cat_func},
    {"fs_rm", "fs_rm <name>, remove file", fs_rm_func},
    {"fs_format", "re-format flash and re-mount fs", fs_reformat_func},
    {"", "", NULL}
};

void init_logStore_hardware(void)
{
    MX_SPI1_Init();
    w25qxx_init(&w25qxx, &hspi1, SPI_CS_GPIO_Port, SPI_CS_Pin);

    /* init log input uart */
}

uint8_t mount_fs(void)
{
    FRESULT fs_res;

    fs_res = f_mount(&fs, "", 1);
    if (fs_res != FR_OK)
    {
        // debug_printf(DEBUG_TAG_ERROR, __LINE__, "mount fs failed once: %d", fs_res);
        if (fs_res == FR_NO_FILESYSTEM)
        {
            // debug_printf(DEBUG_TAG_INFO, __LINE__, "no fs, will make it");
            f_unmount("");
            fs_res = f_mkfs("", &opt, work_sp, W25Q32_SECTOR_SIZE);

            if (fs_res != FR_OK)
            {
                return 0;
            }

            fs_res = f_mount(&fs, "", 1);
            if (fs_res != FR_OK)
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    fs_flag = 1;

    return 1;
}

void register_fs_ops(void)
{
    uint16_t i = 0;

    while (strlen(fs_opts[i].task_name) != 0)
    {
        console_task_register(&fs_opts[i]);
        i++;
    }
}

static int fs_ls_func(int argc, char* argv[])
{
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    FS_CHECK();
    if (argc != 1) {
        PRINTF("too many args\r\n");
        return -1;
    }

    res = f_opendir(&dir, "");                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) 
                break;  /* Break on error or end of dir */
            
            PRINTF("%s\r\n", fno.fname);
        }
        f_closedir(&dir);
    }
    else
    {
        PRINTF("ls failed\r\n");
    }

    return 1;

}

static int fs_mk_func(int argc, char* argv[])
{
    FIL new_file;
    FRESULT res;

    FS_CHECK();

    if (argc != 2) 
    {
        PRINTF("args error\r\n");
        return -1;
    }

    res = f_open(&new_file, argv[1], FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        PRINTF("mkfi fialed\r\n");
        return -1;
    }

    f_close(&new_file);

    return 1;
}

static int fs_w_func(int argc, char* argv[])
{
    FIL new_file;
    FRESULT res;
    UINT wb;

    FS_CHECK();

    if (argc != 3)
    {
        PRINTF("args error\r\n");
        return -1;
    }


    res = f_open(&new_file, argv[1], FA_WRITE | FA_OPEN_ALWAYS);
    if (res != FR_OK)
    {
        PRINTF("open failed\r\n");
        return -1;
    }

    res = f_write(&new_file, argv[2], strlen(argv[2]), &wb);
    if (res != FR_OK)
    {
        f_close(&new_file);
        PRINTF("write filed\r\n");
        return -1;
    }

    PRINTF("write success: %d\r\n", wb);
    f_close(&new_file);

    return 1;
}

static int fs_cat_func(int argc, char* argv[])
{
    FIL new_file;
    FRESULT res;
    UINT rb;
    UINT to_r;
    uint8_t* r_data;

    FS_CHECK();

    if ( argc>3 || argc <2 )
    {
        PRINTF("args error\r\n");
        return -1;
    }


    res = f_open(&new_file, argv[1], FA_READ);
    if (res != FR_OK)
    {
        PRINTF("open failed\r\n");
        return -1;
    }

    if (argc == 3)
        to_r = atoi(argv[2]);
    else
    {
        to_r = f_size(&new_file);
    }

    r_data = malloc(to_r+1);
    memset(r_data, 0, to_r+1);

    res = f_read(&new_file, r_data, to_r, &rb);
    if (res != FR_OK)
    {
        f_close(&new_file);
        PRINTF("read filed\r\n");
        return -1;
    }

    PRINTF("read success: \r\n%s\r\n", r_data);
    f_close(&new_file);

    return 1;
}

static int fs_rm_func(int argc, char* argv[])
{
    FRESULT res;

    FS_CHECK();

    if ( argc !=2 )
    {
        PRINTF("args error\r\n");
        return -1;
    }   

    res = f_unlink(argv[1]);
    if (res != FR_OK)
    {
        PRINTF("remove %s failed\r\n", argv[1]);
        return -1;
    }

    return 1;
}

static int fs_reformat_func(int argc, char* argv[])
{
    W25QXX_result_t ret;

    ret = w25qxx_chip_erase(&w25qxx);

    if (ret == W25QXX_Ok) {
        fs_flag = 0;
        PRINTF("erase flash success\r\n");
    }
    else
        PRINTF("erase flash failed\r\n");

    if (mount_fs()) {
        fs_flag = 1;
        PRINTF("mount fs success\r\n");
    }
    else {
        PRINTF("mount fs failed\r\n");
        fs_flag = 0;
    }

    return 1;
}
