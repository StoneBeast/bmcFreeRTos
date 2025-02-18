/*
 * @Author       : stoneBeast
 * @Date         : 2025-01-22 10:14:12
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-17 13:46:24
 * @Description  : 通用链表管理工具
 */

#ifndef __LINK_LIST_H_
#define __LINK_LIST_H_

#define MAX_LEN(a, b) ((a) > (b) ? (a) : (b))

typedef void* link_list_handle_t;

typedef struct
{
    link_list_handle_t list;
    unsigned short node_number;
    short (*add2list)(link_list_handle_t *list,
                      void *node,
                      unsigned short node_size,
                      void *id,
                      unsigned short id_len);
    void *(*find_by_index)(link_list_handle_t list, short index);
    void *(*find_by_id)(link_list_handle_t list, void *id, unsigned short id_len);
    void *(*find_by_pos)(link_list_handle_t *list, short pos);
    void  (*delete_by_id)(link_list_handle_t *list, void *id);
    void  (*foreach)(link_list_handle_t list, void (*callback)(void* vendor_data));
} link_list_manager;

link_list_manager *link_list_manager_get(void);

#endif
