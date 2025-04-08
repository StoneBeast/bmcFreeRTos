/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-10 16:28:57
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-01 19:01:20
 * @Description  : 日志存储功能功能函数
 */

#include "spi.h"
#include "w25qxx.h"
#include "flashConfig.h"
#include "logStore.h"
#include <stdlib.h>
#include <string.h>
#include "usart.h"
#include <stdio.h>
#include "user_lfs.h"
#include "cmsis_os.h"

#define PRINTF(...)

/* littlefs 配置参数 */
const struct lfs_config cfg = {
    // block device operations
    .read  = user_provided_block_device_read,
    .prog  = user_provided_block_device_prog,
    .erase = user_provided_block_device_erase,
    .sync  = user_provided_block_device_sync,

    // block device configuration
    .read_size      = 1,
    .prog_size      = 256,
    .block_size     = 4096,
    .block_count    = 1024,
    .cache_size     = 4096,
    .lookahead_size = 128,
    .block_cycles   = 500,
};

static void free_fs(void);

#if 0
static void send_stream(uint8_t* data, uint32_t len);

static int fs_ls_func(int argc, char* argv[]);
static int fs_cat_func(int argc, char* argv[]);
static int fs_reformat_func(int argc, char* argv[]);

/* 文件系统操作 */
Task_t fs_opts[] = {
    {"fs_ls", "list file in current directory", fs_ls_func},
    {"fs_cat", "fs_cat <name> [count], display file [count]/full byte", fs_cat_func},
    {"fs_format", "re-format flash and re-mount fs", fs_reformat_func},
    {"", "", NULL}
};

#endif

uint8_t fs_flag = 0;                        /* 文件系统挂载标志位 */
W25QXX_HandleTypeDef w25qxx;                /* w25qxx操作实例 */
lfs_t lfs;                                  /* lfs实例 */
uint8_t rx_buf_0[RX_BUFFER_MAX_LEN] = {0};  /* 接收buffer以及接收长度 */
uint16_t rx_buf_0_len               = 0;
uint16_t rx_buf_0_last_len          = 0;    /* 记录上次接收的长度，用于判断接收是否结束 */
uint8_t rx_buf_1[RX_BUFFER_MAX_LEN] = {0};
uint16_t rx_buf_1_len               = 0;
uint16_t rx_buf_1_last_len          = 0;
uint8_t current_buf                 = 0;    /* 记录当前接收的buffer */
uint8_t start_flag                  = 0;    /* 接收开始标志位 */
uint8_t end_flag                    = 0;    /* 接收结束标志位 */

/*** 
 * @brief 初始化日志记录功能使用到的硬件
 * @return [void]
 */
void init_logStore_hardware(void)
{
    MX_SPI2_Init();
    w25qxx_init(&w25qxx, &hspi2, SPI_CS_GPIO_Port, SPI_CS_Pin);

    /* init log input uart */
    MX_USART3_UART_Init();
}

/*** 
 * @brief 挂载文件系统
 * @return [uint16_t]   返回需要写入的日志文件标号，失败则返回0
 */
uint16_t mount_fs(void)
{
    // TODO: 可以使用直接记录整型数据，而非字符数据的方式
    uint16_t meta_data = 0;
    lfs_file_t index_file;      /* 日志文件实例 */

    /* 尝试挂载文件系统，挂载失败则格式化 */
    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        lfs_format(&lfs, &cfg);
        err = lfs_mount(&lfs, &cfg);
        if (err)
            return 0;
    }

    /* 统计存储介质可用空间 */
    free_fs();

    /* 读取index文件，或许需要写入的文件名 */
    err = lfs_file_open(&lfs, &index_file, ".index", LFS_O_RDWR | LFS_O_CREAT);
    if (err) {
        PRINTF("no file\r\n");
        return 0;
    }

    err = lfs_file_read(&lfs, &index_file, &meta_data, 2);

    meta_data++;

    /* 写入新的标号，覆盖旧的标号 */
    lfs_file_rewind(&lfs, &index_file);
    err = lfs_file_write(&lfs, &index_file, &meta_data, 2);

    lfs_file_close(&lfs, &index_file);

    fs_flag = 1;

    return meta_data;
}

uint16_t get_file_count(void)
{
    int f_res = 0;
    uint16_t file_count = 0;
    lfs_file_t index_file;
    lfs_ssize_t rw_b;

    f_res = lfs_file_open(&lfs, &index_file, ".index", LFS_O_RDWR);
    if (f_res)
        goto FILE_COUNT_END;

    rw_b = lfs_file_read(&lfs, &index_file, &file_count, 2);
    if (rw_b != 2)
        goto FILE_COUNT_END;

    if (file_count == 0)
        goto FILE_COUNT_END;


FILE_COUNT_END:
    lfs_file_close(&lfs, &index_file);
    return file_count;
}

/*** 
 * @brief 获取存储介质可用空间，有需要则重新格式化
 * @return [void]
 */
static void free_fs(void)
{
    lfs_ssize_t fs_size;
    lfs_ssize_t free_size;

    fs_size   = lfs_fs_size(&lfs);
    free_size = (lfs.cfg->block_count * lfs.cfg->block_size - (fs_size * 4096));

    PRINTF("%lu KiB total drive space.\r\n%lu KiB available.\r\n", (lfs.cfg->block_count * lfs.cfg->block_size) / 1024, free_size / 1024);

    /* 剩余的空间少于要求的容量，重新格式化并挂载存储介质 */
    if (free_size / 1024 < MIN_AVAILABLE_SPACE) {
        PRINTF("no more space, re-format fs...\r\n");
        lfs_format(&lfs, &cfg);

        mount_fs();
    }
}

#if 0
/*** 
 * @brief 注册文件系统操作函数
 * @return [void]
 */
void register_fs_ops(void)
{
    uint16_t i = 0;

    while (strlen(fs_opts[i].task_name) != 0)
    {
        console_task_register(&fs_opts[i]);
        i++;
    }
}

/*** 
 * @brief 显示当前目录下所有文件
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        任务执行结果
 */
static int fs_ls_func(int argc, char* argv[])
{
    int f_res;
    lfs_dir_t dir;
    struct lfs_info info;

    if (argc != 1) {
        PRINTF("too many args\r\n");
        return -1;
    }

    f_res = lfs_dir_open(&lfs, &dir, "/");
    if (f_res == 0) {
        for (;;) {
            f_res = lfs_dir_read(&lfs, &dir, &info);
            if (f_res == 0 || info.name[0] == 0)
                break;

            PRINTF("%s\t%luB\r\n", info.name, info.size);
        }
        lfs_dir_close(&lfs, &dir);
    } else {
        PRINTF("open dir failed\r\n");
    }

    return 1;

}

/*** 
 * @brief 读取指定文件
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        任务执行结果
 */
static int fs_cat_func(int argc, char* argv[])
{
    int f_res;
    lfs_file_t log_file;
    char read_buf[READ_BUFFER_MAX_LEN] = {0};
    lfs_ssize_t read_bytes;     /* 读取到的数据长度 */
    UBaseType_t current;        /* 当前任务优先级 */

    if ( argc>3 || argc <2 )
    {
        PRINTF("args error\r\n");
        return -1;
    }

    f_res = lfs_file_open(&lfs, &log_file, argv[1], LFS_O_RDONLY);
    if (f_res != 0) {
        PRINTF("open %s failed\r\n", argv[1]);
        return 1;
    }

    PRINTF("log %s: size: %luB\r\n", argv[1], lfs_file_size(&lfs, &log_file));

    /* 记录当前任务优先级 */
    current = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, configMAX_PRIORITIES-1);
    /* 采用分段读取的方式读取，每次读取READ_BUFFER_MAX_LEN字节 */
    do
    {
        read_bytes           = lfs_file_read(&lfs, &log_file, read_buf, READ_BUFFER_MAX_LEN);
        send_stream((uint8_t *)read_buf, read_bytes);
    } while (read_bytes == READ_BUFFER_MAX_LEN);
    PRINTF("\r\n");
    
    lfs_file_close(&lfs, &log_file);
    vTaskPrioritySet(NULL, current);

    return 1;
}

/*** 
 * @brief 重新格式化并挂载文件系统操作
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        执行结果
 */
static int fs_reformat_func(int argc, char* argv[])
{
    lfs_format(&lfs, &cfg);
    mount_fs();

    return 1;
}

/*** 
 * @brief 文件数据发送函数，方便移植不同的数据发送方式
 * @param data [uint8_t*]   指向数据的指针
 * @param len [uint32_t]    数据长度
 * @return [void]
 */
static void send_stream(uint8_t *data, uint32_t len)
{
    HAL_UART_Transmit(&huart1, data, len, 100);
}
#endif

void USART3_IRQHandler(void)
{
    uint8_t rc;
    if (start_flag == 0 && rx_buf_0_len == 0&& rx_buf_1_len == 0)
        start_flag = 1;

    HAL_UART_Receive(&huart3, &rc, 1, 100);
    if (start_flag == 1 && end_flag ==0 && fs_flag == 1)
    {
        if (0 == current_buf) 
        {
            if (rx_buf_0_len < RX_BUFFER_MAX_LEN)
                rx_buf_0[rx_buf_0_len++] = rc;
        }
        else
        {
            if (rx_buf_1_len < RX_BUFFER_MAX_LEN)
                rx_buf_1[rx_buf_1_len++] = rc;
        }
    }
}
