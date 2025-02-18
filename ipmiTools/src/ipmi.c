/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 16:56:54
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-18 18:05:18
 * @Description  : ipmi功能实现
 */

#include "ipmi.h"
#include "ipmiMessage.h"
#include "ipmiHardware.h"
#include "ipmiCommand.h"
#include <string.h>
#include <stdlib.h>
#include "link_list.h"
#include "uartConsole.h"
#include "console.h"
#include "cmsis_os.h"

//  TODO: 释放rqSeq功能

uint8_t g_local_addr = 0x00;
uint8_t re_seq_arr[64] = {0};
uint8_t g_ipmb_msg[64] = {0};
uint16_t g_ipmb_msg_len = 0;
uint8_t timeout_seq[16] = {0};
uint8_t timeout_seq_count= 0; 
link_list_manager* ipmi_request_manager;
link_list_manager* timeout_request_manager;
link_list_manager* ipmi_res_manager;
QueueHandle_t res_queue;
SemaphoreHandle_t i2c_mutex; 

static uint8_t get_iocal_addr(void)
{
#ifdef IS_BMC
    g_local_addr = IPMI_BMC_ADDR;
#else
    uint8_t ga = read_GA_Pin();
    uint8_t hard_addr = VPX_HARDWARE_ADDR(ga);
    if (!ASSERENT_VPX_HARDWARE_ADDR(hard_addr))
        return 0;

    g_local_addr = VPX_IPMB_ADDR(hard_addr);
#endif
    return 1;
}

static uint8_t init_ipmb(void)
{
    if (!get_iocal_addr())
        goto init_failed;
    
    init_ipmb_i2c(g_local_addr);
    return 1;

init_failed:
    return 0;
}

static uint8_t init_sensor(void)
{
    init_adc();
    return 1;
}

static uint8_t* package_ipmi_msg(const struct ipmi_msg_t* msg)
{
    /* 这里的长度 8 计算了cmp_code */
    uint8_t* pkg = (uint8_t*)malloc(RESPONSE_FORMAT_LEN + msg->data_len);

    pkg[1] = ADD_LUN(msg->netfn, msg->rs_lun);
    pkg[4] = ADD_LUN(msg->rq_seq, msg->rq_lun);
    pkg[5] = msg->cmd;

    return pkg;
}

static void get_checksum(uint8_t* msg, uint16_t len)
{
    uint8_t chk1 = 0;
    uint8_t chk2 = 0;
    uint8_t sum2 = 0;

    chk1 = ((0x100-(msg[0]+msg[1])) % 0x100);
    msg[2] = chk1;
    
    for (uint16_t i = 3; i < len-1; i++)
        sum2 += msg[i];

    chk2 = ((0x100-sum2) % 0x100);
    msg[len-1] = chk2;
}

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

void req_timeout_handler(void* req)
{
    ipmi_req_t* temp = (ipmi_req_t*)req;

    if (temp->msg_ticks + 50 < get_sys_ticks())
    {
        timeout_request_manager->add2list(&(timeout_request_manager->list), temp, sizeof(ipmi_req_t),
                                            &(temp->rq_seq), 1);
        timeout_seq[timeout_seq_count++] = temp->rq_seq;
        PRINTF("TO: %d\r\n", temp->rq_seq);
    }
}

static int req_timeout_task_func(int argc, char* argv[])
{
    //TODO: 请求超时响应处理
    ipmi_request_manager->foreach(ipmi_request_manager->list, req_timeout_handler);

    for (uint8_t i = 0; i < timeout_seq_count; i++)
        ipmi_request_manager->delete_by_id(&(ipmi_request_manager->list), &(timeout_seq[i]));

    timeout_seq_count = 0;
    
    return 1;
}

static int res_handle_task_func(int argc, char* argv[])
{
    /* 处理响应消息 */
    ipmb_recv_t* temp;
    uint8_t rqSeq = 0;

    if (ipmi_res_manager->node_number)
    {
        temp = (ipmb_recv_t*)(ipmi_res_manager->find_by_pos(&(ipmi_res_manager->list), (ipmi_res_manager->node_number)-1));
        rqSeq = ((temp->msg[4])>>2);
        
        xQueueSend(res_queue, temp, portMAX_DELAY);

        ipmi_request_manager->delete_by_id(&(ipmi_request_manager->list), &(rqSeq));
        ipmi_res_manager->delete_by_id(&(ipmi_res_manager->list), &(rqSeq));
        re_seq_arr[rqSeq]= 0; 
    }

    return 1;
}

uint8_t init_bmc(void)
{
    Bg_task_t req_timeout_task = {
        .task = {
            .task_func = req_timeout_task_func,
            .task_name = "req_timeout",
            .task_desc = "find timeout request"
        },
        .time_interval = 5,
        .time_until = 0,
    };

    Bg_task_t res_handle_task = {
        .task = {
            .task_func = res_handle_task_func,
            .task_name = "res_handle_task",
            .task_desc = "handle response"
        },
        .time_interval = 2,
        .time_until = 0,
    };

    if (!init_ipmb())
        return 0;
    init_sensor();
    i2c_mutex = xSemaphoreCreateMutex();
    ipmi_request_manager = link_list_manager_get();
    timeout_request_manager = link_list_manager_get();
    ipmi_res_manager = link_list_manager_get();
    console_backgroung_task_register(&req_timeout_task);
    console_backgroung_task_register(&res_handle_task);

    res_queue = xQueueCreate(5, sizeof(ipmb_recv_t));

    return g_local_addr;
}

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

uint16_t get_ipmc_status(void)
{
    return 0;
}

uint8_t add_msg_list(uint8_t* msg, uint16_t len)
{
    uint8_t rqSeq = 0;
    ipmb_recv_t recv = {
        .msg_len = len,
    };

    if (!check_msg(msg, len))
        return 0;

    memcpy(recv.msg, msg, len);

    rqSeq = ((msg[4])>>2);
    ipmi_res_manager->add2list(&(ipmi_res_manager->list), &recv, sizeof(ipmb_recv_t), &(rqSeq), 1);

    return 1;
}

/*** 
 * @brief 发送ipmi命令: GET_DEVICE_ID
 * @param dev_ipmi_addr [uint8_t]   目标设备ipmb地址
 * @param data_len[out] [uint16_t*] 返回数据包长度
 * @return [uint8_t*]               [free]请求成功返回指向数据包的指针，超时则返回NULL
 */
uint8_t* ipmi_get_device_ID(uint8_t dev_ipmi_addr, uint16_t* data_len)
{
    ipmb_recv_t temp_recv;
    BaseType_t recv_ret;
    uint8_t* ret_data;

    if (!ipmi_request(dev_ipmi_addr, CMD_GET_DEVICE_ID, NULL, 0))
        return NULL;
    
    recv_ret = xQueueReceive(res_queue, &temp_recv, portTICK_PERIOD_MS*WAIT_RESPONSE_MAX);

    if (recv_ret == pdFALSE) /* 接收失败 */
    {
        *data_len = 0;
        return NULL;
    }

    *data_len = RES_DATA_LEN(temp_recv.msg_len);
    ret_data = (uint8_t*)malloc(*data_len);
    memcpy(ret_data, &(temp_recv.msg[RESPONSE_DATA_START]), *data_len);

    return ret_data;
}
