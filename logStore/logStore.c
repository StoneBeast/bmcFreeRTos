/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-10 16:28:57
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-20 17:27:17
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
#include "usart.h"
#include <stdio.h>

//BUG: 后续在读取log时需要分段，否则会导致ram溢出

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

uint8_t rx_buf_0[RX_BUFFER_MAX_LEN] = {0};
uint16_t rx_buf_0_len = 0;
uint8_t rx_buf_1[RX_BUFFER_MAX_LEN] = {0};
uint16_t rx_buf_1_len = 0;
uint8_t current_buf = 0;

void init_logStore_hardware(void)
{
    MX_SPI2_Init();
    w25qxx_init(&w25qxx, &hspi2, SPI_CS_GPIO_Port, SPI_CS_Pin);

    /* init log input uart */
    MX_USART3_UART_Init();
}

uint16_t mount_fs(void)
{
    FRESULT fs_res;
    FIL index_file;
    char index_str[8] = {0};
    uint16_t index_i = 0;
    UINT b_rw = 0;

    fs_res = f_mount(&fs, "", 1);
    if (fs_res != FR_OK)
    {
        if (fs_res == FR_NO_FILESYSTEM)
        {
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

    fs_res = f_open(&index_file, "index", FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
    if (fs_res != FR_OK)
        return 0;
    
    fs_res = f_read(&index_file, index_str, 4, &b_rw);
    if (fs_res != FR_OK)
        return 0;

    if (b_rw == 0)
        index_str[0] = '0';
    index_i = atoi((char *)index_str);
    index_i++;
    sprintf(index_str, "%04d", index_i);

    f_rewind(&index_file);
    fs_res = f_write(&index_file, index_str, 4, &b_rw);
    if (fs_res != FR_OK)
        return 0;

    f_close(&index_file);

    fs_flag = 1;

    return index_i;
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

    PRINTF("read success, %d: \r\n%s\r\n", rb, r_data);
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

void USART3_IRQHandler(void)
{
    uint8_t rc;
    uint32_t isrflags = READ_REG(huart3.Instance->SR);

    if ((isrflags & CONSOLE_IT_RXEN) != RESET)
    {
        HAL_UART_Receive(&huart3, &rc, 1, 100);
        PRINTF("get 3: %c\r\n", rc);
        // if (0 == current_buf) 
        // {
        //     if (rx_buf_0_len < RX_BUFFER_MAX_LEN)
        //         rx_buf_0[rx_buf_0_len++] = rc;
        // }
        // else
        // {
        //     if (rx_buf_1_len < RX_BUFFER_MAX_LEN)
        //         rx_buf_1[rx_buf_1_len++] = rc;
        // }
    }
}
