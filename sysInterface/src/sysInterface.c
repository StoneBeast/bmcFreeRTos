/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-28 18:26:48
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-07-25 14:38:18
 * @Description  : 
 */

#include "hardware.h"
#include "sysInterface.h"
#include "cmsis_os.h"
#include "sysInterface_def.h"
#include "logStore.h"
#include <string.h>
#include "bmc.h"

static uint8_t get_checksum(const uint8_t *msg, uint16_t msg_len);
static uint8_t check_msg(const uint8_t *msg, uint16_t msg_len);
static void get_file_list_handler(void);
static void read_file_handler(uint16_t file_index);
static void get_card_list_handler(void);
static void get_sensor_list_handler(uint8_t ipmi_addr);
static void get_event(void);
static void response(uint8_t* res, uint16_t res_len);

static sys_req_t sys_req_ring[10];
static uint8_t sys_req_ring_head = 0;
static uint8_t sys_req_ring_tail = 0;
#define P_INCREASE(p) ((p==9)?(0):(p+1))

static sys_req_t request_recv_buffer;           /* 接收请求buffer */
static uint8_t send_buffer[BUFFER_LEN] = {0};   /* 发送响应的buffer，最大长度为 BUFFER_LEN */
// TODO: 不确定是否要使用队列，消息缓冲区等也可以考虑
// QueueHandle_t sys_req_queue;                    /* 存放请求的队列 */
QueueHandle_t ack_queue;
extern TaskHandle_t sys_req_handler_task;

#if USE_DEBUG_CMD == 1
    volatile uint8_t debug_read = 0;
#endif // !USE_DEBUG_CMD == 1

/*** 
 * @brief 初始化系统接口使用到的硬件
 * @return [void]
 */
void init_sysInterface(void)
{
    init_status_led();
    init_interface_uart();
    enable_uart_interrupt();
    clear_interface_uart_idel_it_flag();
}

/*** 
 * @brief 请求处理函数
 * @return [void]
 */
void sys_request_handler(void)
{
    //TODO: 有限替换malloc为freertos中的api，减少内存碎片
    sys_req_t req; /* 接收请求 */
    uint32_t ulNotifiedValue;

    // sys_req_queue = xQueueCreate(6, sizeof(sys_req_t));
    ack_queue = xQueueCreate(1, sizeof(sys_req_t));

    while (1)
    {
        // xEventGroupWaitBits(req_event, 1, pdTRUE, pdFALSE, portMAX_DELAY);
        // xQueueReceive(sys_req_queue, &req, portMAX_DELAY);

        xTaskNotifyWait(pdFALSE, 1, &ulNotifiedValue, portMAX_DELAY);

        if (sys_req_ring_head == sys_req_ring_tail) {
            vTaskDelay(20);
            continue;
        }

        memcpy(&req, &(sys_req_ring[sys_req_ring_head]), sizeof(sys_req_t));
        sys_req_ring_head = P_INCREASE(sys_req_ring_head);

        if (req.request_msg[MSG_TYPE_OFFSET] == SYS_MSG_TYPE_REQ) {
            switch (req.request_msg[MSG_CODE_OFFSET]) {
                case SYS_CMD_DEVICE_LIST  :
                    get_card_list_handler();
                    break;
                case SYS_CMD_DEVICE_SENSOR:
                    get_sensor_list_handler(req.request_msg[MSG_DATA_OFFSET]);
                    break;
                case SYS_FILE_LIST        :
                    get_file_list_handler();
                    break;
                case SYS_FILE_READ        :
                    read_file_handler(*(uint16_t*)(&(req.request_msg[MSG_DATA_OFFSET])));
                    break;
                case SYS_FILE_REMOVE      :
                    break;
                case SYS_FILE_REFORMAT    :
                    break;
                case SYS_EVENT_SENSOR_OVER:
                    get_event();
                    break;
                default:
                    break;
            }
        }
    }
}

/*** 
 * @brief 计算校验码
 * @param msg [uint8_t*]        指向消息的指针
 * @param msg_len [uint16_t]    消息长度
 * @return [uint8_t]            校验和
 */
static uint8_t get_checksum(const uint8_t *msg, uint16_t msg_len)
{
    uint16_t sum = 0;
    uint8_t chk = 0;

    for (uint16_t i = 0; i < msg_len; i++)
        sum += msg[i];

    chk = (0x100 - sum % 0x100);

    return chk;
}

/*** 
 * @brief 检查校验和
 * @param msg [uint8_t*]        指向消息的指针
 * @param msg_len [uint16_t]    消息长度
 * @return [uint8_t]            消息完整/不完整 : 0/1
 */
static uint8_t check_msg(const uint8_t *msg, uint16_t msg_len)
{
    uint16_t sum = 0;

#if USE_DEBUG_CMD == 1
    if (0 == memcmp(msg, DEBUG_CMD_PREFIX, strlen(DEBUG_CMD_PREFIX)))
        return 0;
#endif // !USE_DEBUG_CMD == 1

    for (uint16_t i = 0; i < msg_len; i++)
        sum += msg[i];

    if (sum % 0x100 != 0)
        return 1;

    return 0;
}

/*** 
 * @brief 获取文件最大序号处理函数
 * @return [void]
 */
static void get_file_list_handler(void)
{
    uint8_t* ret_msg = NULL;
    /* 获取最大文件编号 */
    uint16_t file_count = get_file_count();
    uint16_t length = MSG_FORMAT_LENGTH + 2;

    ret_msg = malloc(length);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_FILE_LIST;
    ret_msg[MSG_LEN_OFFSET]  = 2;
    memcpy(&ret_msg[MSG_DATA_OFFSET], &file_count, 2);
    
    ret_msg[length - 1] = get_checksum(ret_msg, length - 1);
    response(ret_msg, length);
    /* 释放ret_msg */
    free(ret_msg);
}

/*** 
 * @brief 读取文件处理函数
 * @param file_index [uint16_t]    请求读取的文件标号
 * @return [void]
 */
static void read_file_handler(uint16_t file_index)
{
    uint8_t* ret_msg = NULL;
    char file_name[6] = {0};
    lfs_file_t log_file;
    lfs_ssize_t rb = 0;
    lfs_soff_t file_size;
    uint8_t retry_count = 0;
    BaseType_t ack_ret = 0;
    uint16_t pkg_index = 0;
    sys_req_t req;

    /* 填充响应信息 */
    ret_msg = malloc(256);
    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_FILE_READ;

    /* 拼接文件名 */
    sprintf(file_name, "%04d", file_index);
    lfs_file_open(&lfs, &log_file, file_name, LFS_O_RDONLY);
    file_size = lfs_file_size(&lfs, &log_file);

    /* 每次读取 256-MSG_FORMAT_LENGTH-4-2-1-1 字节数据，如果实际读取的数据小于这个数字则说明文件读取完毕 */
    do
    {
        memset(&(ret_msg[MSG_DATA_OFFSET]), 0, 256-4);

        memcpy(&(ret_msg[MSG_FILE_PKG_FILE_SIZE_OFFSET]), &file_size, MSG_FILE_PKG_FILE_SIZE_LEN);
        memcpy(&(ret_msg[MSG_FILE_PKG_PKG_INDEX_OFFSET]), &pkg_index, MSG_FILE_PKG_PKG_INDEX_LEN);

        /* pkg len: 1Byte, pkg flag: 1Byte, pkg index: 2Byte, file size: 4Byte */
        rb = lfs_file_read(&lfs, &log_file, &(ret_msg[MSG_FILE_PKG_PKG_DATA_OFFSET]), 256 - MSG_FORMAT_LENGTH - MSG_FILE_PKG_FILE_SIZE_LEN - MSG_FILE_PKG_PKG_INDEX_LEN - MSG_FILE_PKG_PKG_FLAG_LEN - MSG_FILE_PKG_PKG_DATA_LEN_LEN);
        ret_msg[MSG_FILE_PKG_PKG_DATA_LEN_OFFSET] = rb;
        if (rb == (256 - MSG_FORMAT_LENGTH -MSG_FILE_PKG_FILE_SIZE_LEN - MSG_FILE_PKG_PKG_INDEX_LEN - MSG_FILE_PKG_PKG_FLAG_LEN - MSG_FILE_PKG_PKG_DATA_LEN_LEN)) {
            /* data域第6个数据存放数据完结标志，0xFF代表未完结，0xAA则代表读取完毕 */
            ret_msg[MSG_FILE_PKG_PKG_FLAG_OFFSET] = 0xFF;
            *((uint16_t*)&(ret_msg[MSG_LEN_OFFSET]))  = (uint16_t)(256 - MSG_FORMAT_LENGTH);
            // chk = get_checksum(ret_msg, 256-2);
            ret_msg[255] = get_checksum(ret_msg, 256-1);
        } else {
            ret_msg[MSG_FILE_PKG_PKG_FLAG_OFFSET]     = 0xAA;
            *((uint16_t *)&(ret_msg[MSG_LEN_OFFSET])) = (uint16_t)(rb + MSG_FILE_PKG_FILE_SIZE_LEN + MSG_FILE_PKG_PKG_INDEX_LEN + MSG_FILE_PKG_PKG_FLAG_LEN + MSG_FILE_PKG_PKG_DATA_LEN_LEN);
            // chk = get_checksum(ret_msg, MSG_FORMAT_LENGTH + rb + MSG_FILE_PKG_FILE_SIZE_LEN + MSG_FILE_PKG_PKG_INDEX_LEN + MSG_FILE_PKG_PKG_FLAG_LEN + MSG_FILE_PKG_PKG_DATA_LEN_LEN);
            // memcpy(&(ret_msg[MSG_FILE_PKG_PKG_DATA_OFFSET+ rb]), &chk, 2);
            ret_msg[MSG_FILE_PKG_PKG_DATA_OFFSET+ rb] = get_checksum(ret_msg, MSG_FORMAT_LENGTH + rb + MSG_FILE_PKG_FILE_SIZE_LEN + MSG_FILE_PKG_PKG_INDEX_LEN + MSG_FILE_PKG_PKG_FLAG_LEN + MSG_FILE_PKG_PKG_DATA_LEN_LEN);
        }

        /* 先清空队列 */
        xQueueReceive(ack_queue, &req, 0);

SEND_FILE_PKG:

    response(ret_msg, 256);
    ack_ret = xQueueReceive(ack_queue, &req, 20 / portTICK_PERIOD_MS);
    if (ack_ret == pdTRUE) { /* 有响应 */
        if (0x06 == req.request_msg[MSG_DATA_OFFSET])
            retry_count = 0;
        else {
            goto RETRY;
        }
    } else {    /* 没有响应，重新发送 */
RETRY:
        if (retry_count >= 3)   /* 超过重试次数，直接退出 */
            break;
        else {
            retry_count ++;
            goto SEND_FILE_PKG;
        }
      }

        pkg_index++;

    } while (rb == (256 - MSG_FORMAT_LENGTH - 1 - 4));

    /* 关闭文件，释放空间 */
    lfs_file_close(&lfs, &log_file);
    free(ret_msg);
}

/*** 
 * @brief 获取板卡列表处理函数
 * @return [void]
 */
static void get_card_list_handler(void)
{
    uint8_t* card_addr_list = NULL;
    uint16_t card_count = 0;
    uint8_t ret_msg[15] = {0};

    card_addr_list = scan_device(&card_count);
    memset(ret_msg, 0, 15);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_CMD_DEVICE_LIST;
    *((uint16_t *)&(ret_msg[MSG_LEN_OFFSET])) = 1 + card_count;
    ret_msg[MSG_DATA_OFFSET] = card_count;
    memcpy(&(ret_msg[MSG_DATA_OFFSET+1]), card_addr_list, card_count);

    ret_msg[MSG_FORMAT_LENGTH + 1 + card_count - 1] = get_checksum(ret_msg, MSG_FORMAT_LENGTH + 1 + card_count -1);
    // memcpy(&(ret_msg[MSG_FORMAT_LENGTH + 1 + card_count - 1]), &chk, 2);

    free(card_addr_list);

    response(ret_msg, MSG_FORMAT_LENGTH+1+card_count);
}

/*** 
 * @brief 获取传感器列表
 * @param ipmi_addr [uint8_t]   目标设备地址
 * @return [void]
 */
static void get_sensor_list_handler(uint8_t ipmi_addr)
{
    uint8_t* ret_msg = NULL;
    uint8_t* sensor_list = NULL;
    uint16_t sensor_list_len = 0;
    // uint16_t chk;

    sensor_list = get_sensor_list(ipmi_addr, &sensor_list_len);
    ret_msg = malloc(MSG_FORMAT_LENGTH+sensor_list_len);
    memset(ret_msg, 0, MSG_FORMAT_LENGTH + sensor_list_len);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_CMD_DEVICE_SENSOR;
    *((uint16_t *)&(ret_msg[MSG_LEN_OFFSET])) = sensor_list_len;
    memcpy(&(ret_msg[MSG_DATA_OFFSET]), sensor_list, sensor_list_len);

    vPortFree(sensor_list);
    ret_msg[MSG_DATA_OFFSET + sensor_list_len] = get_checksum(ret_msg, MSG_DATA_OFFSET + sensor_list_len);
    // memcpy(&(ret_msg[MSG_DATA_OFFSET + sensor_list_len]), &chk, 2);
    response(ret_msg, MSG_FORMAT_LENGTH+sensor_list_len);
    free(ret_msg);
}

/*** 
 * @brief 获取事件
 * @return [void]
 */
static void get_event(void)
{
    uint8_t event_count = 0;                /* 事件个数 */
    sensor_ev_t temp_event;                 /* 临时用来存放取出的事件实例 */
    uint8_t* ret_msg = NULL;                /* 返回消息 */
    uint16_t data_len = 0;                  /* 响应中data域的长度 */
    uint16_t msg_point = MSG_DATA_OFFSET;   /* 响应消息填充指针 */
    uint8_t pop_count = 0;

    event_count = get_event_count();
    ret_msg     = malloc(MSG_FORMAT_LENGTH + 1 + event_count * 30);
    memset(ret_msg, 0, MSG_FORMAT_LENGTH + 1 + event_count * 30);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_EVENT_SENSOR_OVER;

    if (event_count <= 8)
        pop_count = event_count;
    else {
        pop_count = 8;
    }
    ret_msg[msg_point] = pop_count;
    data_len           = 1;
    msg_point++;

    for (uint8_t i = 0; i < pop_count; i++) {
        pop_event(&temp_event);
        ret_msg[msg_point++] = temp_event.addr;
        data_len++;
        ret_msg[msg_point++] = temp_event.sensor_no;
        data_len++;
        ret_msg[msg_point++] = temp_event.is_signed;
        data_len++;
        memcpy(&(ret_msg[msg_point]), &(temp_event.min_val), 2);
        msg_point += 2;
        data_len += 2;
        memcpy(&(ret_msg[msg_point]), &(temp_event.max_val), 2);
        msg_point += 2;
        data_len += 2;
        memcpy(&(ret_msg[msg_point]), &(temp_event.read_val), 2);
        msg_point += 2;
        data_len += 2;
        memcpy(&(ret_msg[msg_point]), &(temp_event.M), 2);
        msg_point += 2;
        data_len += 2;
        memcpy(&(ret_msg[msg_point]), &(temp_event.K2), 2);
        msg_point += 2;
        data_len += 2;
        ret_msg[msg_point++] = temp_event.unit_code;
        data_len++;
        ret_msg[msg_point++] = temp_event.sensor_name_len;
        data_len++;
        memcpy(&(ret_msg[msg_point]), temp_event.sensor_name, temp_event.sensor_name_len);
        msg_point += temp_event.sensor_name_len;
        data_len += temp_event.sensor_name_len;
    }
    memcpy(&(ret_msg[MSG_LEN_OFFSET]), &data_len, 2);
    ret_msg[msg_point] = get_checksum(ret_msg, data_len + MSG_FORMAT_LENGTH);
    // memcpy(&(ret_msg[msg_point]), &chk, 2);

    response(ret_msg, data_len+MSG_FORMAT_LENGTH);
    free(ret_msg);
}

/*** 
 * @brief 发送响应消息
 * @param res [uint8_t*]        指向消息的指针
 * @param res_len [uint16_t]    响应长度
 * @return [void]
 */
static void response(uint8_t *res, uint16_t res_len)
{
    /* 将实际响应的数据复制到发送缓冲中 */
    memset(send_buffer, 0, BUFFER_LEN);
    memcpy(send_buffer, res, res_len);
    /* 停止任务调度，防止发送过程中因任务调度而产生断帧 */
    vTaskSuspendAll();
    interface_uart_send_data(send_buffer, BUFFER_LEN);
    xTaskResumeAll();
}

void USART1_IRQHandler(void)
{
    char rc;
    BaseType_t is_yield = pdFALSE;

    if (get_interface_uart_it_flag(SYSINTERFACE_IT_RXEN) != 0) {
        rc = get_interface_uart_data();
        if (request_recv_buffer.request_len != REQ_MAX_LEN)
            request_recv_buffer.request_msg[request_recv_buffer.request_len++] = rc;
    }

    /* 
        FIXME:  这里req使用了环形队列，而没有使用freertos提供的队列
                这里仅作测试用。
    */
    else if (get_interface_uart_it_flag(SYSINTERFACE_IT_IDLE) != 0) /* idle中断触发，完成一帧数据帧接收 */
    {
        if (0 == check_msg(request_recv_buffer.request_msg, request_recv_buffer.request_len)) {
            if (request_recv_buffer.request_msg[0] == 0x03) {
                xQueueSendFromISR(ack_queue, &request_recv_buffer, &is_yield);
                portYIELD_FROM_ISR(is_yield);
            } else {
                memcpy(&(sys_req_ring[sys_req_ring_tail]), &request_recv_buffer, sizeof(sys_req_t));
                sys_req_ring_tail = P_INCREASE(sys_req_ring_tail);
                // xEventGroupSetBitsFromISR(req_event, 1, NULL);
                // xQueueSendFromISR(sys_req_queue, &request_recv_buffer, &is_yield);
                xTaskNotifyFromISR(sys_req_handler_task, 1, eSetBits, &is_yield);
                portYIELD_FROM_ISR(is_yield);
            }
        }

        request_recv_buffer.request_len = 0;
        clear_interface_uart_idel_it_flag();

        is_yield = pdTRUE;
    }
}
