#include "my_task.h"
#include "stm32f1xx.h"

static uint32_t get_sys_ticks(void)
{
    return HAL_GetTick();
}

static void task_timeout_handler(void *task_handle)
{
    uint32_t now_tick = get_sys_ticks();

    timer_task_t *timer_task = (timer_task_t *)task_handle;
    if (now_tick >= timer_task->time_until) {
        timer_task->task_func();
        timer_task->time_until = timer_task->time_interval + now_tick;
    }
}

link_list_manager* init_task_list(void)
{
    return link_list_manager_get();
}

void timer_task_register(link_list_manager* manager, timer_task_t *task)
{
    task->time_until = task->time_interval + get_sys_ticks();
    manager->add2list(&(manager->list), task, sizeof(timer_task_t), task->task_name, TASK_NAME_MAX_LEN);
}

void task_handler(link_list_manager *manager)
{
    manager->foreach (manager->list, task_timeout_handler);
}
