/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-28 18:26:48
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-04-08 14:49:35
 * @Description  : 
 */

#include "hardware.h"
#include "sysInterface.h"
#include "cmsis_os.h"
#include "sysInterface_def.h"
#include "logStore.h"
#include <string.h>
#include "bmc.h"

static sys_req_t request_recv_buffer;
static uint8_t send_buffer[BUFFER_LEN] = {0};
static uint8_t get_checksum(const uint8_t *msg, uint16_t msg_len);
static uint8_t check_msg(const uint8_t *msg, uint16_t msg_len);
static void get_file_list_handler(void);
static void read_file_handler(uint16_t file_index);
static void get_card_list_handler(void);
static void get_sensor_list_handler(uint8_t ipmi_addr);
static void get_event(void);
static void response(uint8_t* res, uint16_t res_len);

// TODO: 使用互斥量，保证互斥访问
// extern sensor_ev_t g_event_arr[16];
// extern uint8_t event_count;
// extern uint8_t event_arr_p;

// TODO: 不确定是否要使用队列，消息缓冲区等也可以考虑
QueueHandle_t sys_req_queue;

void init_sysInterface(void)
{
    init_status_led();
    init_interface_uart();
    enable_uart_interrupt();
}

void sys_request_handler(void)
{
    sys_req_t req;

    sys_req_queue = xQueueCreate(2, sizeof(sys_req_t));

    while (1)
    {
        xQueueReceive(sys_req_queue, &req, portMAX_DELAY);

        if (check_msg(req.request_msg, req.request_len))    /* 请求数据损坏 */
            continue;

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

static uint8_t get_checksum(const uint8_t *msg, uint16_t msg_len)
{
    uint8_t sum = 0;
    uint8_t chk = 0;

    for (uint16_t i = 0; i < msg_len; i++)
        sum += msg[i];

    chk = ((0x100 - sum) % 0x100);

    return chk;
}

static uint8_t check_msg(const uint8_t *msg, uint16_t msg_len)
{
    uint8_t sum = 0;

    for (uint16_t i = 0; i < msg_len; i++)
        sum += msg[i];

    if (sum % 0x100 != 0)
        return 1;

    return 0;
}

static void get_file_list_handler(void)
{
    uint8_t* ret_msg = NULL;
    uint16_t file_count = get_file_count();
    uint16_t length = MSG_FORMAT_LENGTH + 2;

    ret_msg = malloc(length);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_FILE_LIST;
    ret_msg[MSG_LEN_OFFSET]  = 2;
    memcpy(&ret_msg[MSG_DATA_OFFSET], &file_count, 2);

    ret_msg[length - 1] = get_checksum(ret_msg, length - 1);
    response(ret_msg, length);
    free(ret_msg);
}

static void read_file_handler(uint16_t file_index)
{
    uint8_t* ret_msg = NULL;
    char file_name[6] = {0};
    lfs_file_t log_file;
    lfs_ssize_t rb = 0;

    ret_msg = malloc(256);
    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_FILE_READ;

    sprintf(file_name, "%04d", file_index);
    lfs_file_open(&lfs, &log_file, file_name, LFS_O_RDONLY);

    do
    {
        memset(&(ret_msg[MSG_DATA_OFFSET]), 0, 256-MSG_FORMAT_LENGTH-1);
        rb = lfs_file_read(&lfs, &log_file, &(ret_msg[MSG_DATA_OFFSET + 1]), 256 - MSG_FORMAT_LENGTH - 1);

        if (rb == (256 - MSG_FORMAT_LENGTH - 1)) {
            ret_msg[MSG_DATA_OFFSET] = 0xFF;
            *((uint16_t*)&(ret_msg[MSG_LEN_OFFSET]))  = (256 - MSG_FORMAT_LENGTH);
            ret_msg[255] = get_checksum(ret_msg, 256-1);
        } else {
            ret_msg[MSG_DATA_OFFSET]                  = 0xAA;
            *((uint16_t *)&(ret_msg[MSG_LEN_OFFSET])) = rb + 1;
            ret_msg[MSG_FORMAT_LENGTH + rb - 1]       = get_checksum(ret_msg, MSG_FORMAT_LENGTH + rb - 1);
        }

        response(ret_msg, 256);

    } while (rb == (256 - MSG_FORMAT_LENGTH - 1));

    lfs_file_close(&lfs, &log_file);
    free(ret_msg);
}

static void get_card_list_handler(void)
{
    uint8_t* card_addr_list = NULL;
    uint16_t card_count = 0;
    uint8_t* ret_msg = NULL;

    card_addr_list = scan_device(&card_count);
    ret_msg = malloc(MSG_FORMAT_LENGTH+1+card_count);
    memset(ret_msg, 0, MSG_FORMAT_LENGTH + 1 + card_count);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_CMD_DEVICE_LIST;
    *((uint16_t *)&(ret_msg[MSG_LEN_OFFSET])) = 1 + card_count;
    ret_msg[MSG_DATA_OFFSET] = card_count;
    memcpy(&(ret_msg[MSG_DATA_OFFSET+1]), card_addr_list, card_count);

    ret_msg[MSG_FORMAT_LENGTH + 1 + card_count - 1] = get_checksum(ret_msg, MSG_FORMAT_LENGTH + 1 + card_count -1);

    free(card_addr_list);

    response(ret_msg, MSG_FORMAT_LENGTH+1+card_count);
    free(ret_msg);
}

static void get_sensor_list_handler(uint8_t ipmi_addr)
{
    uint8_t* ret_msg = NULL;
    uint8_t* sensor_list = NULL;
    uint16_t sensor_list_len = 0;

    sensor_list = get_sensor_list(ipmi_addr, &sensor_list_len);
    ret_msg = malloc(MSG_FORMAT_LENGTH+sensor_list_len);
    memset(ret_msg, 0, MSG_FORMAT_LENGTH + sensor_list_len);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_CMD_DEVICE_SENSOR;
    *((uint16_t *)&(ret_msg[MSG_LEN_OFFSET])) = sensor_list_len;
    memcpy(&(ret_msg[MSG_DATA_OFFSET]), sensor_list, sensor_list_len);

    free(sensor_list);
    ret_msg[MSG_DATA_OFFSET + sensor_list_len] = get_checksum(ret_msg, MSG_DATA_OFFSET + sensor_list_len);
    response(ret_msg, MSG_FORMAT_LENGTH+sensor_list_len);
    free(ret_msg);
}

static void get_event(void)
{
    uint8_t event_count = 0;
    sensor_ev_t temp_event;
    // uint8_t event_ret = 0;
    uint8_t* ret_msg = NULL;
    uint16_t data_len = 0;
    uint16_t msg_point = MSG_DATA_OFFSET;

    event_count = get_event_count();
    ret_msg     = malloc(MSG_FORMAT_LENGTH + 1 + event_count * 27);
    memset(ret_msg, 0, MSG_FORMAT_LENGTH + 1 + event_count * 27);

    ret_msg[MSG_TYPE_OFFSET] = SYS_MSG_TYPE_RES;
    ret_msg[MSG_CODE_OFFSET] = SYS_EVENT_SENSOR_OVER;
    ret_msg[msg_point]       = event_count;
    data_len = 1;
    msg_point++;

    for (uint8_t i = 0; i < event_count; i++) {
        pop_event(&temp_event);
        ret_msg[msg_point++] = temp_event.addr;
        data_len++;
        ret_msg[msg_point++] = temp_event.sensor_no;
        data_len++;
        ret_msg[msg_point++] = temp_event.min_val;
        data_len++;
        ret_msg[msg_point++] = temp_event.max_val;
        data_len++;
        ret_msg[msg_point++] = temp_event.read_val;
        data_len++;
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

    response(ret_msg, data_len+MSG_FORMAT_LENGTH);
    free(ret_msg);
}

static void response(uint8_t *res, uint16_t res_len)
{
    memset(send_buffer, 0, BUFFER_LEN);
    memcpy(send_buffer, res, res_len);
    vTaskSuspendAll();
    interface_uart_send_data(send_buffer, BUFFER_LEN);
    xTaskResumeAll();
}

void USART1_IRQHandler(void)
{
    char rc;

    if (get_interface_uart_it_flag(SYSINTERFACE_IT_RXEN) != 0) {
        rc = get_interface_uart_data();
        if (request_recv_buffer.request_len != REQ_MAX_LEN)
            request_recv_buffer.request_msg[request_recv_buffer.request_len++] = rc;
    }

    if (get_interface_uart_it_flag(SYSINTERFACE_IT_IDLE) != 0) /* idle中断触发，完成一帧数据帧接收 */
    {
        xQueueSendFromISR(sys_req_queue, &request_recv_buffer, NULL);
        request_recv_buffer.request_len = 0;
        clear_interface_uart_idel_it_flag();
    }
}
