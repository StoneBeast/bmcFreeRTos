#include "my_task.h"
#include "stm32f1xx.h"

/*** 
 * @brief 获取系统tick
 * @return [uint32_t]   系统tick
 */
static uint32_t get_sys_ticks(void)
{
    return HAL_GetTick();
}

/*** 
 * @brief 任务超时处理
 * @param task_handle [void*]   指向定时任务实例的指针 
 * @return [void]
 */
static void task_timeout_handler(void *task_handle)
{
    uint32_t now_tick = get_sys_ticks();

    timer_task_t *timer_task = (timer_task_t *)task_handle;
    if (now_tick >= timer_task->time_until) {   /* 超时则执行超时函数 */
        timer_task->task_func();
        timer_task->time_until = timer_task->time_interval + now_tick;
    }
}

/*** 
 * @brief 获取链表实例
 * @return [link_list_manager*] 链表实例
 */
link_list_manager* init_task_list(void)
{
    return link_list_manager_get();
}

/*** 
 * @brief 注册定时任务
 * @param manager [link_list_manager*]  目标链表
 * @param task [timer_task_t*]          任务实例
 * @return [void]
 */
void timer_task_register(link_list_manager* manager, timer_task_t *task)
{
    /* 初始化timer_until成员 */
    task->time_until = task->time_interval + get_sys_ticks();
    manager->add2list(&(manager->list), task, sizeof(timer_task_t), task->task_name, TASK_NAME_MAX_LEN);
}

/*** 
 * @brief 轮询并处理超时任务
 * @param manager [link_list_manager*]  任务链表
 * @return [void]
 */
void task_handler(link_list_manager *manager)
{
    manager->foreach (manager->list, task_timeout_handler);
}
