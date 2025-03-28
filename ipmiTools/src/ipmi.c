/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-02-06 16:56:54
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-24 18:40:32
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
#include "cmsis_os.h"
#include "ipmiSDR.h"
#include "ipmiEvent.h"
#include "cmsis_os.h"

//  TODO: 释放rqSeq功能

fru_t g_fru;
sdr_index_info_t g_sdr_index;
uint8_t g_local_addr = 0x00;                    /* 本机地址 */
uint8_t re_seq_arr[64] = {0};                   /* req seq占用状态记录 */
uint8_t g_ipmb_msg[64] = {0};                   /* ipmb消息接收数组 */
uint16_t g_ipmb_msg_len = 0;                    /* ipmb消息接收长度 */
uint8_t timeout_seq[16] = {0};                  /* 记录超时请求seq */
uint8_t timeout_seq_count= 0;                   /* 超时请求个数 */
link_list_manager* ipmi_request_manager;        /* 管理发送出的请求的链表 */
link_list_manager* ipmi_res_manager;            /* 管理响应的链表 */
QueueHandle_t res_queue;                        /* 将相应信息从接收task传递到处理task */
QueueHandle_t event_queue;                      /* 事件通知队列 */

static int scan_device(int argc, char* argv[]);
static int info_device(int argc, char* argv[]);
static void get_fru_info(void);
uint8_t get_device_id_msg_handler(fru_t* fru, uint8_t* msg);
static uint8_t* get_device_sdr(uint8_t ipmi_addr, uint16_t record_id, uint8_t* sdr_len);
static uint8_t* get_device_sdr_info(uint8_t ipmi_addr);
static int get_sensor_list_task_func(int argc, char* argv[]);
static int update_sensor_data_task_func(int argc, char* argv[]);
static void event_handler(void* arg);

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
static int res_handle_task_func(int argc, char* argv[])
{
    /* 处理响应消息 */
    ipmb_recv_t* temp;
    uint8_t rqSeq = 0;

    if (ipmi_res_manager->node_number)
    {
        /* 查找响应信息 */
        temp = (ipmb_recv_t*)(ipmi_res_manager->find_by_pos(&(ipmi_res_manager->list), (ipmi_res_manager->node_number)-1));
        rqSeq = ((temp->msg[4])>>2);
        
        /* 通过消息队列发送消息到请求线程 */
        xQueueSend(res_queue, temp, portMAX_DELAY);

        /* 删除请求和响应，并释放rqseq */
        ipmi_request_manager->delete_by_id(&(ipmi_request_manager->list), &(rqSeq));
        ipmi_res_manager->delete_by_id(&(ipmi_res_manager->list), &(rqSeq));
        re_seq_arr[rqSeq]= 0; 
    }

    return 1;
}

/*** 
 * @brief 初始化bmc
 * @return [uint8_t]    本机地址
 */
uint8_t init_bmc(void)
{
    /* 添加后台任务 */
    Bg_task_t res_handle_task = {
        .task = {
            .task_func = res_handle_task_func,
            .task_name = "res_handle_task",
            .task_desc = "handle response"
        },
        .time_interval = 2,
        .time_until = 0,
    };

    Bg_task_t update_sensor_task = {
        .task = {
            .task_func = update_sensor_data_task_func,
            .task_name = "update_sensor",
            .task_desc = "update sensor"
        },
        .time_interval = 5003,
        .time_until = 0,
    };

    /* 添加主动任务 */
    Task_t scan_device_task = {
        .task_func = scan_device,
        .task_name = "device",
        .task_desc = "display all device"
    };

    Task_t info_device_task = {
        .task_func = info_device,
        .task_name = "info",
        .task_desc = "info <addr> display device info"
    };

    Task_t get_sensor_list_task = {
        .task_func = get_sensor_list_task_func,
        .task_name = "sensor",
        .task_desc = "sensor <ipmi addr>"
    };

    if (!init_ipmb())
        return 0;

    init_sensor();
    get_fru_info();
    index_sdr(&g_sdr_index);
    ipmi_request_manager = link_list_manager_get();
    ipmi_res_manager = link_list_manager_get();
    console_task_register(&scan_device_task);
    console_task_register(&info_device_task);
    console_task_register(&get_sensor_list_task);
    console_backgroung_task_register(&res_handle_task);
    console_backgroung_task_register(&update_sensor_task);

    res_queue = xQueueCreate(5, sizeof(ipmb_recv_t));
    event_queue = xQueueCreate(5, sizeof(sensor_ev_t));
    xTaskCreate(event_handler, "event", 256, NULL, 1, NULL);

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
        temp_e.addr = msg[3];
        temp_e.sensor_no = msg[RESPONSE_DATA_START+EVENT_BODY_SENSOR_NO_OFFSET];
        temp_e.min_val = data_conversion(msg[RESPONSE_DATA_START+EVENT_BODY_MIN_VAL_OFFSET], &(msg[RESPONSE_DATA_START+EVENT_BODY_START_M_OFFSET]));
        temp_e.read_val = data_conversion(msg[RESPONSE_DATA_START+EVENT_BODY_READ_VAL_OFFSET], &(msg[RESPONSE_DATA_START+EVENT_BODY_START_M_OFFSET]));
        temp_e.max_val = data_conversion(msg[RESPONSE_DATA_START+EVENT_BODY_MAX_VAL_OFFSET], &(msg[RESPONSE_DATA_START+EVENT_BODY_START_M_OFFSET]));
        memset(temp_e.sensor_name, 0, 16);
        memcpy(temp_e.sensor_name, &(msg[RESPONSE_DATA_START+EVENT_BODY_NAME_OFFSET]), msg[RESPONSE_DATA_START+EVENT_BODY_NAME_LEN_OFFSET]);
        xQueueSendFromISR(event_queue, &temp_e, NULL);
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
static int scan_device(int argc, char* argv[])
{
    uint8_t i = 0;
    fru_t* ret_fru;

    PRINTF("=================== Device List ===================\r\n");
    PRINTF("Slot\tIPMB Addr\tDevice Name\t\r\n");

    /* 本机信息 */
    PRINTF("%-4s\t0x%-8x\t%s\r\n", "----", g_local_addr, "local");

    /* 插槽数量事先设定 */
    for (i = 0; i < SLOT_COUNT; i++)
    {
        ret_fru = ipmi_get_device_ID(VPX_IPMB_ADDR((VPX_BASE_HARDWARE_ADDR+1+i)));

        if (ret_fru)
        {
            PRINTF("%-4d\t0x%-8x\tDev%d\r\n", i+1, VPX_IPMB_ADDR((VPX_BASE_HARDWARE_ADDR+1+i)), i);
        }
    }

    return 1;
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
 * @brief 显示指定设备信息
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        执行结果
 */
static int info_device(int argc, char* argv[])
{
    fru_t* res_fru;             /* 接收处理完成后的设备信息 */
    char* temp_p = NULL;        /* 用于 字符串-整型 转换 */
    uint8_t ipmb_addr = 0x00;   /* 目标设备地址 */

    if (argc != 2)  /* 判断参数个数 */
    {
        PRINTF("Usage: \r\n\tinfo [addr]\r\n");
        return -1;
    }

    /* 判断地址合法性 */
    ipmb_addr = strtoul(argv[1], &temp_p, 0);
    if ((!ASSERENT_VPX_IPMB_ADDR(ipmb_addr)) && (ipmb_addr != g_local_addr))
    {
        PRINTF("addr invalid\r\n");
        return -1;
    }

    /* 获取设备信息 */
    if (ipmb_addr == g_local_addr)
        res_fru = &g_fru;
    else
        res_fru = ipmi_get_device_ID(ipmb_addr);

    if (res_fru == NULL)
        PRINTF("request send error\r\n");
    else {  /* 打印信息 */
        PRINTF("\r\n");
        PRINTF("Solt: %d\r\n", res_fru->slot);
        PRINTF("Device ID: 0x%02\r\n", res_fru->device_id);
        PRINTF("Device %sProvides SDRs\r\n", ((res_fru->provides_sdr == 0x00) ? "don't ":" "));
        PRINTF("Device Revision: %d\r\n", res_fru->device_rev);
        PRINTF("Device Available: %s\r\n", (res_fru->device_avaliable == 0x00) ? "normal operation":"busy");
        PRINTF("Firmware Revision: %d.%d\r\n", res_fru->firmware_rev_maj, res_fru->firmware_rev_min);
        PRINTF("IPMI Version: %d.%d\r\n", (res_fru->ipmi_ver)&0x03, (res_fru->ipmi_ver) >> 4);
        PRINTF("Additional Device Support:\r\n");
        /* additional */
        if ((res_fru->additional) & (0x01))
            PRINTF("\tSensor Device\r\n");

        if ((res_fru->additional) & (0x01 << 1))
            PRINTF("\tSDR Repository Device\r\n");
        
        if ((res_fru->additional) & (0x01 << 2))
            PRINTF("\tSEL Device\r\n");

        if ((res_fru->additional) & (0x01 << 3))
            PRINTF("\tFRU Inventory Device\r\n");

        if ((res_fru->additional) & (0x01 << 4))
            PRINTF("\tIPMB Event Receiver\r\n");
            
        if ((res_fru->additional) & (0x01 << 5))
            PRINTF("\tIPMB Event Generator\r\n");

        if ((res_fru->additional) & (0x01 << 6))
            PRINTF("\tBridge\r\n");

        if ((res_fru->additional) & (0x01 << 7))
            PRINTF("\tChassis Device\r\n");

        PRINTF("Manufacturer ID: 0x%x\r\n"
               "Product ID: %x\r\n"
               "Auxiliary Firmware Revison Info: %02xH %02xH %02xH %02xH \r\n",
               res_fru->manuf_id,
               res_fru->prodect_id,
               ((uint8_t*)(&(res_fru->aux_firmware_rev)))[0],
               ((uint8_t*)(&(res_fru->aux_firmware_rev)))[1],
               ((uint8_t*)(&(res_fru->aux_firmware_rev)))[2],
               ((uint8_t*)(&(res_fru->aux_firmware_rev)))[3]
              );

        if (ipmb_addr != g_local_addr)
            free(res_fru);
    }

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
    uint16_t start_addr;

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
    
        *res_len = temp_recv.msg_len - RESPONSE_FORMAT_LEN;
        p_res_data = malloc(*res_len);
    
        memcpy(p_res_data, (temp_recv.msg)+RESPONSE_DATA_START, *res_len);
    }
    else
    {
        if (record_id == 0x0000)
            target_sdr_index = 0;
        else {
            for (uint8_t i = 0; i < g_sdr_index.sdr_count; i++)
            {
                if(record_id == g_sdr_index.info[i].id) {
                    target_sdr_index = i;
                    break;
                }
            }
        }

        start_addr = g_sdr_index.info[target_sdr_index].addr;
        *res_len = 2 + (g_sdr_index.info)[target_sdr_index].len;

        p_res_data = malloc(*res_len);

        if (target_sdr_index == (g_sdr_index.sdr_count - 1))
            ((uint16_t*)(p_res_data))[0] = 0x0000;
        else
            ((uint16_t*)(p_res_data))[0] = g_sdr_index.info[target_sdr_index+1].id;

        read_flash(start_addr, ((*res_len)-2), &(p_res_data[2]));
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
static int get_sensor_list_task_func(int argc, char* argv[])
{
    uint8_t ipmi_addr;
    char* temp_p = NULL;
    uint8_t* sdr_info;
    uint8_t sensor_num = 0;
    uint8_t sdr_data_len;
    uint8_t* temp_res_data;
    char* temp_name;
    uint8_t temp_name_len = 0;
    uint16_t next_id = 0;
    char* temp_val_str;

    if (argc != 2) {
        PRINTF("wrong parameters\r\n");
        PRINTF("Usage: tsensor [ipmi addr]\r\n");
        return -1;
    }

    ipmi_addr = strtoul(argv[1], &temp_p, 0);

    if (ipmi_addr == g_local_addr) 
        sensor_num = g_sdr_index.sdr_count;

    else {
        sdr_info = get_device_sdr_info(ipmi_addr);
        if (sdr_info == NULL)
            return -1;
    
        sensor_num = sdr_info[0];
    }

    if (sensor_num == 0) {
        PRINTF("No Sensor in 0x%02x\r\n", ipmi_addr);
        return 1;
    }

    PRINTF("------------------ Sensor List ------------------\r\n");
    PRINTF("%-9s%-10s%-10s%-11s%-10s%s\r\n\r\n", "NO.", "Addr", "Type", "Value", "Name", "Entity ID");

    do
    {
        temp_res_data = get_device_sdr(ipmi_addr, next_id, &sdr_data_len);

        next_id = ((uint16_t*)temp_res_data)[0];
        temp_name_len = temp_res_data[2+SDR_ID_SRT_TYPE_LEN_OFFSET];
        temp_name = malloc(temp_name_len + 1);
        memset(temp_name, 0, temp_name_len + 1);
        memcpy(temp_name, &(temp_res_data[2+SDR_ID_STR_BYTE_OFFSET]), temp_name_len);
        temp_val_str = get_val_str(&temp_res_data[2+SDR_SENSOR_UNITS_1_OFFSET]);

        osThreadYield();
        PRINTF("%02d\t%1s0x%02x\t%3s0x%02x\t%5s%s\t%s\t%2s0x%02x\r\n",  temp_res_data[2+SDR_SENSOR_NUMBER_OFFSET],
                                                                        " ",
                                                                        temp_res_data[2+SDR_SENSOR_OWNER_ID_OFFSET],
                                                                        " ",
                                                                        temp_res_data[2+SDR_SENSOR_TYPE_OFFSET],
                                                                        " ",
                                                                        temp_val_str,
                                                                        temp_name,
                                                                        " ",
                                                                        temp_res_data[2+SDR_ENTITY_ID_OFFSET]);
        free(temp_val_str);
        free(temp_name);
        free(temp_res_data);
    } while (0 != next_id);
    
    PRINTF("\r\n");

    return 1;

}

/*** 
 * @brief 获取fru info，占位
 * @return [void]
 */
static void get_fru_info(void)
{
    g_fru.ipmb_addr = g_local_addr;
    g_fru.hard_addr = (g_local_addr>>1);
    g_fru.slot = 0;
    strcpy(g_fru.device_name, DEVICE_NAME);
    g_fru.device_id = DEVICE_ID;
    g_fru.prodect_id = PRODUCT_ID;
    g_fru.device_rev = DEVICE_REVISION;
    g_fru.firmware_rev_maj = FIRMWARE_REVISION_MAJOR;
    g_fru.firmware_rev_min = FIRMWARE_REVISION_MINOR;
    g_fru.ipmi_ver = IPMI_VERSION;
    g_fru.additional = ADDITIONAL;
    g_fru.manuf_id = MANUFACTURER_ID;
    g_fru.aux_firmware_rev = AUXILIARY_FIRMWARE_REV;
}

/*** 
 * @brief 更新传感器数据任务函数
 * @param argc [int]    参数个数
 * @param argv [char*]  参数列表
 * @return [int]        执行结果
 */
static int update_sensor_data_task_func(int argc, char* argv[])
{
    uint8_t data[4];
    uint8_t ht_data[2];
    sensor_ev_t temp_p;
    uint8_t temp_sdr_M[6];
    uint8_t temp_str_len = 0;

    /* 读取传感器数据 */
    data[0] = read_sdr1_sensor_data();
    data[1] = read_sdr2_sensor_data();
    data[2] = read_sdr3_sensor_data();
    data[3] = read_sdr4_sensor_data();

    for (uint8_t i = 0; i < 4; i++)
    {
        /* 向sdr更新数据 */
        write_flash((g_sdr_index.info[i].addr)+SDR_NORMAL_READING_OFFSET, 1, &(data[i]));

        /* 判断是否为正常数据 */
        read_flash((g_sdr_index.info[i].addr)+SDR_NORMAL_MAX_READING_OFFSET, 2, ht_data);
        if((data[i] < ht_data[1]) || (data[i] > ht_data[0]))
        {
            read_flash((g_sdr_index.info[i].addr)+SDR_M_OFFSET, 6, temp_sdr_M);
            temp_p.addr = g_local_addr;
            temp_p.read_val = data_conversion((short)(data[i]), temp_sdr_M);
            temp_p.max_val = data_conversion((short)(ht_data[0]), temp_sdr_M);
            temp_p.min_val = data_conversion((short)(ht_data[1]), temp_sdr_M);
            read_flash((g_sdr_index.info[i].addr)+SDR_SENSOR_NUMBER_OFFSET, 1, &(temp_p.sensor_no)); 
            memset(temp_p.sensor_name, 0, 16);
            read_flash((g_sdr_index.info[i].addr)+SDR_ID_SRT_TYPE_LEN_OFFSET, 1, &temp_str_len);
            read_flash((g_sdr_index.info[i].addr)+SDR_ID_STR_BYTE_OFFSET, temp_str_len, (uint8_t*)(temp_p.sensor_name));

            /* 上报事件 */
            xQueueSend(event_queue, &temp_p, portMAX_DELAY);
        }
    }

    return 1;
}

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
        if (pdTRUE == xQueueReceive(event_queue, &temp, portMAX_DELAY))
            PRINTF("w: 0x%02x\t%s\tl: %f\tr: %f\tu: %f\r\n", temp.addr, temp.sensor_name, temp.min_val, temp.read_val, temp.max_val);
    }
}
