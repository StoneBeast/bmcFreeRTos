/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 16:56:54
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-07-28 14:58:15
 * @Description  : ipmi功能实现
 */

#include "ipmi.h"
#include "ipmiMessage.h"
#include "ipmiHardware.h"
#include "ipmiCommand.h"
#include <string.h>
#include <stdlib.h>
#include "link_list.h"
#include "cmsis_os.h"
#include "ipmiSDR.h"
#include "ipmiEvent.h"
#include "cmsis_os.h"
#include "my_task.h"
#include <stdio.h>

//  TODO: 释放rqSeq功能

uint8_t g_local_addr = 0x00;                    /* 本机地址 */
uint8_t re_seq_arr[64] = {0};                   /* req seq占用状态记录 */
uint8_t g_ipmb_msg[64] = {0};                   /* ipmb消息接收数组 */
uint16_t g_ipmb_msg_len = 0;                    /* ipmb消息接收长度 */
uint8_t timeout_seq[16] = {0};                  /* 记录超时请求seq */
uint8_t timeout_seq_count= 0;                   /* 超时请求个数 */
link_list_manager* ipmi_request_manager;        /* 管理发送出的请求的链表 */
link_list_manager* ipmi_res_manager;            /* 管理响应的链表 */
QueueHandle_t res_queue;                        /* 将相应信息从接收task传递到处理task */
SemaphoreHandle_t event_queue_mutex;
static Sdr_index_t sdr_index;

static sensor_ev_t g_event_arr[EVENT_MAX_COUNT];
static uint8_t event_arr_tail = 0;
static uint8_t event_arr_head = 0;
volatile static uint8_t battery_flag = 0;

extern link_list_manager *g_timer_task_manager;
#if USE_DEBUG_CMD == 1
extern volatile uint8_t debug_read;
#endif // !USE_DEBUG_CMD == 1

uint8_t* scan_device(uint16_t* device_count);
uint8_t get_device_id_msg_handler(fru_t* fru, uint8_t* msg);
static uint8_t* get_device_sdr(uint8_t ipmi_addr, uint16_t record_id, uint8_t* sdr_len);
static uint8_t* get_device_sdr_info(uint8_t ipmi_addr);
static void update_sensor_data_task_func(void);

/*** 
 * @brief 获取本地地址
 * @return [uint8_t]    执行结果
 */
static uint8_t get_iocal_addr(void)
{
    g_local_addr = IPMI_BMC_ADDR;
    return 1;
}

/*** 
 * @brief 初始化ipmb
 * @return [uint8_t]    初始化结果
 */
static uint8_t init_ipmb(void)
{
    if (!get_iocal_addr())
        goto init_failed;
    
    init_ipmb_i2c(g_local_addr);
    return 1;

init_failed:
    return 0;
}

/*** 
 * @brief 初始化传感器
 * @return [uint8_t]    初始化结果
 */
static uint8_t init_sensor(void)
{
    /* 初始化adc */
    init_adc();
    init_inter_bus();
    init_temp_sensor();
    return 1;
}

/*** 
 * @brief   封装ipmi消息
 * @param msg [ipmi_msg_t*] 存放具体信息的结构
 * @return [uint8_t]        [free]返回封装完成的消息
 */
static uint8_t* package_ipmi_msg(const struct ipmi_msg_t* msg)
{
    /* 这里的长度 8 计算了cmp_code */
    uint8_t* pkg = (uint8_t*)malloc(RESPONSE_FORMAT_LEN + msg->data_len);

    pkg[1] = ADD_LUN(msg->netfn, msg->rs_lun);
    pkg[4] = ADD_LUN(msg->rq_seq, msg->rq_lun);
    pkg[5] = msg->cmd;

    return pkg;
}

/*** 
 * @brief 计算两个checksum，并放入msg中
 * @param msg [uint8_t*]    数据包
 * @param len [uint16_t]    数据包长度
 * @return [void]
 */
static void get_checksum(uint8_t* msg, uint16_t len)
{
    uint8_t chk1 = 0;
    uint8_t chk2 = 0;
    uint8_t sum2 = 0;

    /* chk1是前2个字节的校验和 */
    chk1 = ((0x100-(msg[0]+msg[1])) % 0x100);
    msg[2] = chk1;
    
    for (uint16_t i = 3; i < len-1; i++)
        sum2 += msg[i];

    /* chk2是之后所有数据的校验和 */    
    chk2 = ((0x100-sum2) % 0x100);
    msg[len-1] = chk2;
}

/*** 
 * @brief 检查校验和是否正确
 * @param msg [uint8_t*]    请求/响应数据
 * @param len [uint16_t]    数据长度
 * @return [uint8_t]        检验结果
 */
static uint8_t check_msg(uint8_t* msg, uint16_t len)
{
    uint16_t sum = 0;
    for (uint16_t i = 0; i < 3; i++)
        sum += msg[i];
    
    if (sum % 0x100 != 0)
        return 0;

    sum = 0;
    for (uint16_t i = 3; i < len; i++)
        sum += msg[i];
    if (sum % 0x100 != 0)
        return 0;

    return 1;
}

/*** 
 * @brief 生成rqseq
 * @return [uint8_t]    生成的rqseq
 */
static uint8_t generic_rqseq(void)
{
    for (uint8_t i = 0; i < 64; i++)
    {
        if (re_seq_arr[i] == 0)
        {
            re_seq_arr[i] = 1;
            return i;
        }
    }

    return 0xFF;
}

/*** 
 * @brief 发送ipmi消息
 * @param msg [uint8_t*]    
 * @param len [uint16_t]    
 * @return [uint8_t]    发送结果
 */
static uint8_t send_ipmi_msg(uint8_t* msg, uint16_t len)
{
    uint8_t send_ret;
    UBaseType_t cur_priority;

    cur_priority = uxTaskPriorityGet(NULL);
    
    /* 设置最高优先级，防止发送被中断 */
    vTaskPrioritySet(NULL, configMAX_PRIORITIES-1);
    send_ret = send_i2c_msg(msg, len);
    /* 恢复优先级 */
    vTaskPrioritySet(NULL, cur_priority);

    return send_ret;
}

/*** 
 * @brief 处理响应任务函数
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        处理结果
 */
static void res_handle_task_func(void)
{
    /* 处理响应消息 */
    ipmb_recv_t* temp;
    uint8_t rqSeq = 0;

    if (ipmi_res_manager->node_number)
    {
        /* 查找响应信息 */
        temp = (ipmb_recv_t*)(ipmi_res_manager->find_by_pos(&(ipmi_res_manager->list), (ipmi_res_manager->node_number)-1));
        rqSeq = ((temp->msg[4])>>2);
        
        if (temp != NULL) {
            /* 通过消息队列发送消息到请求线程 */
            xQueueSend(res_queue, temp, pdMS_TO_TICKS(300));
        }

        /* 删除请求和响应，并释放rqseq */
        ipmi_request_manager->delete_by_id(&(ipmi_request_manager->list), &(rqSeq));
        ipmi_res_manager->delete_by_id(&(ipmi_res_manager->list), &(rqSeq));
        re_seq_arr[rqSeq]= 0; 
    }
}

/*** 
 * @brief 初始化bmc
 * @return [uint8_t]    本机地址
 */
uint8_t init_bmc(void)
{
    /* 添加后台任务 */
    timer_task_t res_handle_task = {
        .task_name = "res_handler",
        .task_func = res_handle_task_func,
        .time_interval = 2
    };

    timer_task_t update_sensor_task = {
        .task_name = "update_sensor",
        .task_func = update_sensor_data_task_func,
        .time_interval = 5003
    };

    if (!init_ipmb())
        return 0;

    init_sensor();
    // verify_sdr();
    // get_fru_info();
    // index_sdr(&g_sdr_index);
    init_sdr(&sdr_index);
    ipmi_request_manager = link_list_manager_get();
    ipmi_res_manager = link_list_manager_get();

    timer_task_register(g_timer_task_manager, &res_handle_task);
    timer_task_register(g_timer_task_manager, &update_sensor_task);

    res_queue = xQueueCreate(5, sizeof(ipmb_recv_t));
    event_queue_mutex = xSemaphoreCreateMutex();

    return g_local_addr;
}

/*** 
 * @brief 发送ipmi请求
 * @param rs_sa [uint8_t]       目标地址
 * @param NetFn_CMD [uint16_t]  命令
 * @param data [uint8_t*]       请求体数据
 * @param data_len [uint16_t]   数据长度
 * @return [uint8_t]            请求结果
 */
uint8_t ipmi_request(uint8_t rs_sa, uint16_t NetFn_CMD, uint8_t* data, uint16_t data_len)
{
    ipmi_req_t req;
    uint8_t* msg_pkg = NULL;
    uint8_t send_ret = 0;

    FILL_IN_MSG_TYPE(req, NetFn_CMD, rs_sa, g_local_addr, data, data_len);

    msg_pkg = package_ipmi_msg(&req);
    msg_pkg[0] = req.rs_sa;
    msg_pkg[3] = req.rq_sa;
    memcpy(msg_pkg+6, req.data, req.data_len);

    get_checksum(msg_pkg, REQUEST_FORMAT_LEN+req.data_len);

    /* 发送信息 */
    send_ret = send_ipmi_msg(msg_pkg, REQUEST_FORMAT_LEN+req.data_len);
    
    /* 添加到请求链表中 */
    if (send_ret)
    {
        req.msg_ticks = get_sys_ticks();
        ipmi_request_manager->add2list(&(ipmi_request_manager->list), &req, 
                                        sizeof(ipmi_req_t), &(req.rq_seq), 1);
    }

    free(msg_pkg);

    /* 返回 */
    return send_ret;
}

/*** 
 * @brief 发送ipmi响应
 * @param rq_sa [uint8_t]       请求源地址
 * @param NetFn_CMD [uint16_t]  响应命令
 * @param cmpl_code [uint8_t]   完成状态码
 * @param data [uint8_t*]       响应数据
 * @param data_len [uint16_t]   响应数据长度
 * @return [uint8_t]            响应发送结果
 */
uint8_t ipmi_response(uint8_t rq_sa, uint16_t NetFn_CMD, uint8_t cmpl_code, uint8_t* data, uint16_t data_len)
{
    ipmi_resp_t resp;
    uint8_t* msg_pkg = NULL;

    FILL_IN_MSG_TYPE(resp.msg, NetFn_CMD, g_local_addr, rq_sa, data, data_len);

    msg_pkg = package_ipmi_msg(&(resp.msg));
    msg_pkg[0] = resp.msg.rq_sa;
    msg_pkg[3] = resp.msg.rs_sa;
    msg_pkg[6] = cmpl_code;
    memcpy(msg_pkg + 6 + 1, resp.msg.data, resp.msg.data_len);

    get_checksum(msg_pkg, RESPONSE_FORMAT_LEN + resp.msg.data_len);

    /* 发送信息 */
    send_ipmi_msg(msg_pkg, RESPONSE_FORMAT_LEN + resp.msg.data_len);
    /* 从请求列表中删除该请求 */

    free(msg_pkg);

    /* 返回 */
    return 1;
}

void push_event(sensor_ev_t ev)
{
    xSemaphoreTake(event_queue_mutex, portMAX_DELAY);

    if (((event_arr_tail+1) % EVENT_MAX_COUNT) != event_arr_head) {
        // event_arr_tail++;
        memcpy(&g_event_arr[event_arr_tail], &ev, sizeof(sensor_ev_t));
        event_arr_tail++;
        event_arr_tail %= EVENT_MAX_COUNT;
    }
    xSemaphoreGive(event_queue_mutex);
}

void push_event_from_isr(sensor_ev_t ev)
{
    xSemaphoreTakeFromISR(event_queue_mutex, NULL);

    if (((event_arr_tail + 1) % EVENT_MAX_COUNT) != event_arr_head) {
        memcpy(&g_event_arr[event_arr_tail], &ev, sizeof(sensor_ev_t));
        event_arr_tail++;
        event_arr_tail %= EVENT_MAX_COUNT;
    }

    xSemaphoreGiveFromISR(event_queue_mutex, NULL);
}

uint8_t pop_event(sensor_ev_t *ret)
{
    xSemaphoreTake(event_queue_mutex, portMAX_DELAY);

    if (event_arr_head != event_arr_tail) {
        memcpy(ret, &(g_event_arr[event_arr_head]), sizeof(sensor_ev_t));
        event_arr_head++;
        event_arr_head %= EVENT_MAX_COUNT;

        xSemaphoreGive(event_queue_mutex);

        return 0;
    }

    xSemaphoreGive(event_queue_mutex);
    return 1;
}

uint8_t pop_event_from_isr(sensor_ev_t *ret)
{
    xSemaphoreTakeFromISR(event_queue_mutex, NULL);
    if (event_arr_head != event_arr_tail) {
        memcpy(ret, &(g_event_arr[event_arr_head]), sizeof(sensor_ev_t));
        event_arr_head++;
        event_arr_head %= EVENT_MAX_COUNT;

        xSemaphoreGiveFromISR(event_queue_mutex, NULL);

        return 0;
    }
    xSemaphoreGiveFromISR(event_queue_mutex, NULL);
    return 1;
}

uint8_t get_event_count(void)
{
    uint8_t ret_count = 0;

    xSemaphoreTake(event_queue_mutex, portMAX_DELAY);
    
    ret_count = (event_arr_tail+EVENT_MAX_COUNT-event_arr_head) % EVENT_MAX_COUNT;
    xSemaphoreGive(event_queue_mutex);

    return ret_count;
}

uint8_t get_event_count_from_isr(void)
{
    uint8_t ret_count = 0;

    xSemaphoreTakeFromISR(event_queue_mutex, NULL);
    ret_count = (event_arr_tail + EVENT_MAX_COUNT - event_arr_tail) % EVENT_MAX_COUNT;
    xSemaphoreGiveFromISR(event_queue_mutex, NULL);

    return ret_count;
}

uint16_t get_ipmc_status(void)
{
    return 0;
}

/*** 
 * @brief 将返回数据添加到响应管理链表中
 * @param msg [uint8_t*]    响应数据
 * @param len [uint16_t]    数据长度
 * @return [uint8_t]        处理结果
 */
uint8_t add_msg_list(uint8_t* msg, uint16_t len)
{
    uint8_t rqSeq = 0;
    ipmb_recv_t recv = {
        .msg_len = len,
    };
    sensor_ev_t temp_e;

    if (!check_msg(msg, len))
        return 0;

    if (((msg[1] >> 2) == 0x3F) && (msg[5] == 0xFF))
    {
        /* 地址 */
        temp_e.addr = msg[3];
        /* 传感器编号 */
        temp_e.sensor_no = msg[RESPONSE_DATA_START+EVENT_BODY_SENSOR_NO_OFFSET];
        temp_e.is_signed = msg[RESPONSE_DATA_START + EVENT_BODY_IS_SIGNED_OFFSET];
        /* min, max, raw */
        memcpy(&(temp_e.min_val), &(msg[RESPONSE_DATA_START + EVENT_BODY_MIN_VAL_OFFSET]), 2);
        memcpy(&(temp_e.max_val), &(msg[RESPONSE_DATA_START + EVENT_BODY_MAX_VAL_OFFSET]), 2);
        memcpy(&(temp_e.read_val), &(msg[RESPONSE_DATA_START + EVENT_BODY_READ_VAL_OFFSET]), 2);

        /* M, K2 */
        memcpy(&(temp_e.M), &(msg[RESPONSE_DATA_START + EVENT_BODY_ARG_M_OFFSET]), 2);
        memcpy(&(temp_e.K2), &(msg[RESPONSE_DATA_START + EVENT_BODY_ARG_K2_OFFSET]), 2);
        // get_M_K2(&(temp_e.M), &(temp_e.K2), &(msg[RESPONSE_DATA_START + 5]));

        /* unit code */
        temp_e.unit_code = msg[RESPONSE_DATA_START+EVENT_BODY_UNIT_CODE_OFFSET];

        /* name, name len */
        memset(temp_e.sensor_name, 0, 16);
        temp_e.sensor_name_len = msg[RESPONSE_DATA_START + EVENT_BODY_NAME_LEN_OFFSET];
        memcpy(temp_e.sensor_name, &(msg[RESPONSE_DATA_START + EVENT_BODY_NAME_OFFSET]), temp_e.sensor_name_len);

        push_event_from_isr(temp_e);
    }
    else
    {
        memcpy(recv.msg, msg, len);
    
        rqSeq = ((msg[4])>>2);
        ipmi_res_manager->add2list(&(ipmi_res_manager->list), &recv, sizeof(ipmb_recv_t), &(rqSeq), 1);
    }

    return 1;
}

/*** 
 * @brief 发送ipmi命令: GET_DEVICE_ID
 * @param dev_ipmi_addr [uint8_t]   目标设备ipmb地址
 * @return [fru_t*]                 [free]请求成功返回指向数据包的指针，超时则返回NULL
 */
fru_t* ipmi_get_device_ID(uint8_t dev_ipmi_addr)
{
    ipmb_recv_t temp_recv;
    BaseType_t recv_ret;
    fru_t* ret_fru;

    if (!ipmi_request(dev_ipmi_addr, CMD_GET_DEVICE_ID, NULL, 0))
        return NULL;
    
    recv_ret = xQueueReceive(res_queue, &temp_recv, portTICK_PERIOD_MS*WAIT_RESPONSE_MAX);

    if (recv_ret == pdFALSE) /* 接收失败 */
    {
        // *data_len = 0;
        return NULL;
    }

    ret_fru = malloc(sizeof(fru_t));
    get_device_id_msg_handler(ret_fru, &(temp_recv.msg[RESPONSE_DATA_START]));
    /* 补充数据 */
    ret_fru->ipmb_addr = dev_ipmi_addr;
    ret_fru->hard_addr = (dev_ipmi_addr >> 1);
    ret_fru->slot = (ret_fru->hard_addr - VPX_BASE_HARDWARE_ADDR);

    return ret_fru;
}

/*** 
 * @brief device task函数，扫描扫描总线上的设备
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        执行结果
 */
uint8_t* scan_device(uint16_t* device_count)
{
    uint8_t i = 0;
    fru_t* ret_fru;
    uint8_t* addr_list = NULL;
    uint16_t temp_count = 0;

    addr_list = malloc(SLOT_COUNT+1);
    addr_list[temp_count++] = g_local_addr;

    for (i = 0; i < SLOT_COUNT; i++) {
        ret_fru = ipmi_get_device_ID(VPX_IPMB_ADDR((VPX_BASE_HARDWARE_ADDR + 1 + i)));

        if (ret_fru) {
            addr_list[temp_count++] = VPX_IPMB_ADDR((VPX_BASE_HARDWARE_ADDR + 1 + i));
        }
    }

    *device_count = temp_count;

    return addr_list;
}

/*** 
 * @brief 处理返回数据，并存储到fru_t结构体中
 * @param fru[out] [fru_t*] 指向返回数据的指针
 * @param msg [uint8_t*]    返回数据(固定长度(15))
 * @return [uint8_t]        处理结果
 */
uint8_t get_device_id_msg_handler(fru_t* fru, uint8_t* msg)
{
    fru->device_id = msg[0];
    fru->device_rev = (msg[1] & 0x03);
    fru->provides_sdr = (msg[1] >> 7);
    fru->device_avaliable = (msg[2] >> 7);
    fru->firmware_rev_maj = (msg[2] & 0x80);
    fru->firmware_rev_min = msg[3];
    fru->ipmi_ver = msg[4];
    fru->additional = msg[5];
    fru->manuf_id = (((uint32_t)msg[6]) | (((uint32_t)msg[7]) << 8) | (((uint32_t)msg[8]) << 16));
    fru->prodect_id = ((uint16_t)msg[9]) | (((uint16_t)msg[10]) << 8);
    fru->aux_firmware_rev = ( ((uint32_t)msg[11]) | 
                              (((uint32_t)msg[12]) << 8) | 
                              (((uint32_t)msg[13]) << 16) | 
                              (((uint32_t)msg[14]) << 24) );

    return 1;
}


/*** 
 * @brief 获取目标设备的指定sdr记录
 * @param ipmi_addr [uint8_t]       目标设备ipmb地址
 * @param record_id [uint16_t]      指定sdr id
 * @param res_len[out] [uint8_t*]   获取的sdr长度 
 * @return [uint8_t*]               [free]指向sdr数据的指针
 */
static uint8_t* get_device_sdr(uint8_t ipmi_addr, uint16_t record_id, uint8_t* res_len)
{
    uint8_t req_ret;
    uint8_t req_data[GET_SDR_REQ_LEN];
    BaseType_t recv_ret;
    ipmb_recv_t temp_recv;
    uint8_t* p_res_data;
    uint8_t target_sdr_index = 0x00;
    Res_sdr_t temp_r_sdr;
    // uint16_t start_addr;

    if (ipmi_addr != g_local_addr)
    {
        /* 编辑请求数据，可以在请求数据中设置偏移以及希望读取的长度，暂未提供修改 */
        req_data[0] = 0;
        req_data[1] = 0;
        req_data[2] = (uint8_t)(record_id & 0x00FF);
        req_data[3] = (uint8_t)((record_id & 0xFF00) >> 8);
        req_data[4] = 0;
        req_data[5] = 0xff;
    
        req_ret = ipmi_request(ipmi_addr, CMD_GET_DEVICE_SDR, req_data, GET_SDR_REQ_LEN);
        if (!req_ret)
            return NULL;
    
        recv_ret = xQueueReceive(res_queue, &temp_recv, portTICK_PERIOD_MS*WAIT_RESPONSE_MAX);
    
        if (recv_ret == pdFALSE) /* 接收失败 */
            return NULL;

        *res_len   = sizeof(Res_sdr_t);
        p_res_data = pvPortMalloc(*res_len);
        /* next id */
        temp_r_sdr.id = temp_recv.msg[RESPONSE_DATA_START];
        /* dev_addr */
        temp_r_sdr.sdr.dev_addr = temp_recv.msg[RESPONSE_DATA_START+1];
        /* sdr id */
        temp_r_sdr.sdr.sdr_id = temp_recv.msg[RESPONSE_DATA_START+2];
        /* sensor type */
        temp_r_sdr.sdr.sensor_type = temp_recv.msg[RESPONSE_DATA_START+3];
        /* data unit code */
        temp_r_sdr.sdr.data_unit_code = temp_recv.msg[RESPONSE_DATA_START+4];
        /* is data signed */
        temp_r_sdr.sdr.is_data_signed = temp_recv.msg[RESPONSE_DATA_START + 5];
        /* read data */
        memcpy(&(temp_r_sdr.sdr.read_data), &(temp_recv.msg[RESPONSE_DATA_START+6]), 2);
        /* higher th */
        memcpy(&(temp_r_sdr.sdr.higher_threshold), &(temp_recv.msg[RESPONSE_DATA_START+8]), 2);
        /* lower th */
        memcpy(&(temp_r_sdr.sdr.lower_threshold), &(temp_recv.msg[RESPONSE_DATA_START+10]), 2);
        /* arg M */
        memcpy(&(temp_r_sdr.sdr.argM), &(temp_recv.msg[RESPONSE_DATA_START+12]), 2);
        /* arg K2 */
        memcpy(&(temp_r_sdr.sdr.argK2), &(temp_recv.msg[RESPONSE_DATA_START+14]), 2);
        /* sensor name len */
        temp_r_sdr.sdr.name_len = temp_recv.msg[RESPONSE_DATA_START+16];
        /* sensor name */
        memcpy(temp_r_sdr.sdr.sensor_name, &(temp_recv.msg[RESPONSE_DATA_START + 17]), temp_r_sdr.sdr.name_len);

        memcpy(p_res_data, &temp_r_sdr, sizeof(Res_sdr_t));
        ((Res_sdr_t *)p_res_data)->sdr.sensor_read = (void*)0xF0F0F0F0;
    }
    else
    {
        if (record_id == 0x0000)
            target_sdr_index = 0;
        else {
            for (uint8_t i = 0; i < sdr_index.sdr_count; i++)
            {
                if(record_id == sdr_index.p_sdr_list[i]->sdr_id) {
                    target_sdr_index = i;
                    break;
                }
            }
        }

        /* 非法id */
        if (target_sdr_index == sdr_index.sdr_count) {
            *res_len = 0;
            p_res_data = NULL;

            return p_res_data;
        }

        *res_len = sizeof(Res_sdr_t);
        p_res_data = pvPortMalloc(sizeof(Res_sdr_t));

        /* 最后一个sdr */
        if (target_sdr_index == (sdr_index.sdr_count - 1)) {
            ((Res_sdr_t*)p_res_data)->id = 0;
        } else {
            ((Res_sdr_t *)p_res_data)->id = sdr_index.p_sdr_list[target_sdr_index+1]->sdr_id;
        }
        memcpy(&(((Res_sdr_t *)p_res_data)->sdr), sdr_index.p_sdr_list[target_sdr_index], sizeof(Sdr_t));

        // start_addr = g_sdr_index.info[target_sdr_index].addr;
        // *res_len = 2 + (g_sdr_index.info)[target_sdr_index].len;

        // p_res_data = malloc(*res_len);

        // if (target_sdr_index == (g_sdr_index.sdr_count - 1))
        //     ((uint16_t*)(p_res_data))[0] = 0x0000;
        // else
        //     ((uint16_t*)(p_res_data))[0] = g_sdr_index.info[target_sdr_index+1].id;

        // read_flash(start_addr, ((*res_len)-2), &(p_res_data[2]));
    }

    return p_res_data;
}

/*** 
 * @brief 获取目标设备的sdr信息
 * @param ipmi_addr [uint8_t]   目标设备的ipmb地址  
 * @return [uint8_t*]           [free]指向sdr info的指针
 */
static uint8_t* get_device_sdr_info(uint8_t ipmi_addr)
{
    uint8_t req_ret = 0;
    BaseType_t recv_ret;
    ipmb_recv_t temp_recv;
    uint8_t* p_ret_data;

    req_ret = ipmi_request(ipmi_addr, CMD_GET_DEVICE_SDR_INFO, NULL, 0);
    if (!req_ret)
        return NULL;

    recv_ret = xQueueReceive(res_queue, &temp_recv, portTICK_PERIOD_MS*WAIT_RESPONSE_MAX);

    if (recv_ret == pdFALSE) /* 接收失败 */
        return NULL;

    p_ret_data = malloc(GET_SDR_INFO_RES_LEN);
    memcpy(p_ret_data, (temp_recv.msg)+RESPONSE_DATA_START, GET_SDR_INFO_RES_LEN);

    return p_ret_data;
}

/*** 
 * @brief 获取，处理并打印出目标设备传感器信息
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        执行结束状态
 */
uint8_t* get_sensor_list(uint8_t ipmi_addr, uint16_t* ret_data_len)
{
    uint8_t* sdr_info;
    uint8_t sensor_num = 0;
    uint8_t sdr_data_len;
    Res_sdr_t* temp_res_data;
    uint16_t next_id = 0;
    uint8_t* ret_data;
    uint16_t temp_point = 0;

    if (ipmi_addr == g_local_addr) 
        sensor_num = sdr_index.sdr_count;

    else {
        sdr_info = get_device_sdr_info(ipmi_addr);
        if (sdr_info == NULL)
            sensor_num = 0;
        else {
            sensor_num = sdr_info[0];
        }
    }

    if (sensor_num == 0) {
        *ret_data_len = 1;
        ret_data = pvPortMalloc(1);
        ret_data[0] = 0;
        return ret_data;
    }

    /* 这里把name的长度限制到了16Byte */
    ret_data = pvPortMalloc(1 + sensor_num * sizeof(Sdr_t));
    memset(ret_data, 0, 1 + sensor_num * sizeof(Sdr_t));
    ret_data[0] = sensor_num;
    temp_point = 1;

    do
    {
        temp_res_data = (Res_sdr_t*)get_device_sdr(ipmi_addr, next_id, &sdr_data_len);
        if (temp_res_data == NULL) {
            continue;
        }

        next_id = temp_res_data->id;
        // memcpy(&(ret_data[temp_point]), &(temp_res_data->sdr), sizeof(Sdr_t));
        // temp_point += sizeof(Sdr_t);

        /* number 1Byte */
        ret_data[temp_point++] = temp_res_data->sdr.sdr_id;
        
        /* signed 1Byte */
        ret_data[temp_point++] = temp_res_data->sdr.is_data_signed;

        /* raw data 2Byte */
        memcpy(&(ret_data[temp_point]), &(temp_res_data->sdr.read_data), 2);
        temp_point += 2;
        // ret_data[temp_point++] = temp_res_data[2 + SDR_NORMAL_READING_OFFSET];
        // ret_data[temp_point++] = temp_res_data[2 + SDR_NORMAL_READING_HIGH_OFFSET];

        /* M 2Byte K 2Byte */
        // get_M_K2(&temp_M, &temp_K2, &temp_res_data[2 + SDR_SENSOR_UNITS_1_OFFSET]);
        memcpy(&(ret_data[temp_point]), &(temp_res_data->sdr.argM), 2);
        temp_point += 2;
        memcpy(&(ret_data[temp_point]), &(temp_res_data->sdr.argK2), 2);
        temp_point += 2;

        /* 单位 */
        ret_data[temp_point++] = temp_res_data->sdr.data_unit_code;

        /* name len */
        ret_data[temp_point++] = temp_res_data->sdr.name_len;
        // if(temp_res_data[2 + SDR_ID_SRT_TYPE_LEN_OFFSET] > 16)
        //     ret_data[temp_point++] = 16;
        // else {
        //     ret_data[temp_point++] = temp_res_data[2 + SDR_ID_SRT_TYPE_LEN_OFFSET];
        // }

        /* name nByte */
        memcpy(&(ret_data[temp_point]), &(temp_res_data->sdr.sensor_name), ret_data[temp_point-1]);

        temp_point += ret_data[temp_point - 1];

        // free(temp_res_data);
        vPortFree(temp_res_data);
    } while (0 != next_id);
    
    *ret_data_len = temp_point;

    return ret_data;

}

/*** 
 * @brief 更新传感器数据任务函数
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        执行结果
 */
static void update_sensor_data_task_func(void)
{
    uint16_t data[SENSOR_COUNT];
    sensor_ev_t temp_p;
    uint8_t is_sig;
    uint8_t is_over = 0;
    uint8_t i = 0;

    /* 读取传感器数据 */
    for (i=0; i<sdr_index.sdr_count; i++) {
        data[i] = sdr_index.p_sdr_list[i]->sensor_read();
    }

#if USE_DEBUG_CMD == 1
    if (debug_read == 1) {
        for (uint8_t i = 0; i < 4; i++)
        {
            // PRINTF("ADC%d: %.3fv\n", i, ((float)(data[i]))/4096*3.3);
            printf("A");
            printf("ADC%d: %.3fv\r\n", i, ((float)(data[i]))/4096*3.3);
        }
        printf("\r\n");
    }
#endif // !USE_DEBUG_CMD == 1

    for (uint8_t i = 0; i < sdr_index.sdr_count; i++) {

        if ((i+1 == 0x02) && (battery_flag == 1)) { /* 已经测量过电池 */
            continue;
        } else if ((i+1 == 0x02) && (battery_flag == 0)) {
            battery_flag = 1;
            close_battery();
        }

        /* 向sdr更新数据 */
        sdr_setData(&sdr_index, i+1, data[i]);

        /* 判断是否为正常数据 */
        is_sig = sdr_index.p_sdr_list[i]->is_data_signed;
        if (is_sig) {
            if (((short)(sdr_index.p_sdr_list[i]->higher_threshold)) < ((short)(data[i])) || ((short)(sdr_index.p_sdr_list[i]->lower_threshold)) > ((short)(data[i])))
                is_over = 1;
            else
                is_over = 0;
        } else {
            if ((sdr_index.p_sdr_list[i]->higher_threshold) < data[i] || (sdr_index.p_sdr_list[i]->lower_threshold) > data[i])
                is_over = 1;
            else
                is_over = 0;
        }

        if(is_over)
        {
            if ((i+1 == 0x02) && (battery_flag == 1)) {
                battery_warn();
            }

            /* 地址 */
            temp_p.addr = g_local_addr;

            /* 传感器编号 */
            temp_p.sensor_no = sdr_index.p_sdr_list[i]->sdr_id;

            temp_p.is_signed = sdr_index.p_sdr_list[i]->is_data_signed;

            /* min */
            temp_p.min_val = sdr_index.p_sdr_list[i]->lower_threshold;

            /* max */
            temp_p.max_val = sdr_index.p_sdr_list[i]->higher_threshold;

            /* raw */
            temp_p.read_val = data[i];

            /* M, K2 */
            temp_p.M = sdr_index.p_sdr_list[i]->argM;
            temp_p.K2 = sdr_index.p_sdr_list[i]->argK2;

            /* unit code*/
            temp_p.unit_code = sdr_index.p_sdr_list[i]->data_unit_code;

            /* name, name len */
            temp_p.sensor_name_len = sdr_index.p_sdr_list[i]->name_len;
            strncpy(temp_p.sensor_name, sdr_index.p_sdr_list[i]->sensor_name, temp_p.sensor_name_len);

            /* 上报事件 */
            push_event(temp_p);
        }
    }
}

#if 0

/*** 
 * @brief 事件处理任务函数
 * @param arg [void*]    任务函数参数
 * @return [void]
 */
static void event_handler(void* arg)
{
    sensor_ev_t temp;

    while (1)
    {
        /* 接收并处理事件数据 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        push_event(temp);

        if (pdTRUE == xQueueReceive(event_queue, &temp, portMAX_DELAY)) {
            PRINTF("w: 0x%02x\t%s\tl: %f\tr: %f\tu: %f\r\n", temp.addr, temp.sensor_name, temp.min_val, temp.read_val, temp.max_val);
            push_event(temp);

                event_count++;
                if (event_count == 17)
                    event_count = 16;

                memcpy(g_event_arr[event_arr_p], temp, sizeof(sensor_ev_t));

                event_arr_p++;
                event_arr_p %= 16;
        }
    }
}
#endif //!0
