/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-01-22 10:14:12
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-02-05 09:40:48
 * @Description  : 通用链表管理工具
 */

#include <stdlib.h>
#include <string.h>
#include "link_list.h"

typedef struct link_node
{
    short index;
    void *id;
    unsigned short id_len;
    void *vendor_data;
    unsigned short vendor_data_size;
    struct link_node *next;
} link_list;

static short link_list_add(link_list_handle_t *list, 
                           void *node, 
                           unsigned short node_size,
                           void *id, 
                           unsigned short id_len)
{
    link_list *temp_node = (link_list *)malloc(sizeof(struct link_node));
    link_list *link_p = (link_list *)(*list);

    if (link_p->next == NULL)
    {
        temp_node->index = 0;
    }
    else
    {
        temp_node->index = (++(link_p->next->index));
    }

    temp_node->id_len = id_len;
    temp_node->id = malloc(id_len);
    memcpy(temp_node->id, id, id_len);

    temp_node->vendor_data_size = node_size;
    temp_node->vendor_data = malloc(node_size);
    memcpy(temp_node->vendor_data, node, node_size);

    temp_node->next = link_p->next;
    link_p->next = temp_node;
    ((link_list_manager *)list)->node_number++;

    return temp_node->index;
}

static void *link_list_find_by_index(link_list_handle_t list, short index)
{
    link_list *list_p = (link_list *)list;

    if (list_p->next->index < index || list_p->next == NULL)
    {
        return NULL;
    }
    else
    {
        link_list *node_p = list_p->next;
        for (short int i = list_p->next->index; i >= 0; i--)
        {
            if (i == index)
            {
                return node_p->vendor_data;
            }
            node_p = node_p->next;
        }

        return NULL;
    }
}

static void *link_list_find_by_id(link_list_handle_t list, void *id, unsigned short id_len)
{
    link_list *list_p = (link_list *)list;

    if (list_p->next == NULL)
    {
        return NULL;
    }
    else
    {
        link_list *node_p = list_p;
        while (node_p->next != NULL)
        {
            if (0 == memcmp(id, node_p->next->id, MAX_LEN(id_len, node_p->next->id_len)))
            {
                return node_p->next->vendor_data;
            }
            node_p = node_p->next;
        }

        return NULL;
    }
}

static void* link_list_find_by_pos(link_list_handle_t *list, short pos)
{
    link_list* list_p = (link_list*)(*list);
    short i = 0;

    if (pos < ((link_list_manager*)list)->node_number)
    {
        list_p = list_p->next;

        while (i != pos)
        {
            list_p = list_p->next;
            i++;
        }

        return list_p->vendor_data;
    }

    return NULL; 
}

static void link_list_delete_by_id(link_list_handle_t *list, void *id)
{
    link_list* list_p = (link_list*)(*list);

    if (list_p->next == NULL)
    {
        return ;
    }
    else
    {
        link_list *node_p = list_p;
        while (node_p->next != NULL)
        {
            if (0 == memcmp(id, node_p->next->id, node_p->next->id_len))
            {
                link_list *temp_node = node_p->next;
                node_p->next = node_p->next->next;

                free(temp_node);

                ((link_list_manager*)list)->node_number--;

                return ;
            }
            node_p = node_p->next;
        }

        return ;
    }
}

static void link_list_foreach(link_list_handle_t list, void (*func)(void* vendor_data))
{
    link_list *list_p = (link_list *)list;

    if (list_p->next == NULL)
    {
        return;
    }
    else
    {
        link_list *node_p = list_p;
        while (node_p->next != NULL)
        {
            func(node_p->next->vendor_data);
            node_p = node_p->next;
        }

        return;
    }
}

link_list_manager *link_list_manager_get(void)
{
    link_list_manager *manager = (link_list_manager *)malloc(sizeof(link_list_manager));

    link_list *list = (link_list *)malloc(sizeof(link_list));
    list->next = NULL;
    list->vendor_data = NULL;

    manager->list = (link_list_handle_t *)list;

    manager->node_number = 0;
    manager->add2list = link_list_add;
    manager->find_by_index = link_list_find_by_index;
    manager->find_by_id = link_list_find_by_id;
    manager->find_by_pos = link_list_find_by_pos;
    manager->delete_by_id = link_list_delete_by_id;
    manager->foreach = link_list_foreach;

    return manager;
}
