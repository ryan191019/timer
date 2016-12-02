
/*
 * @file mtc_linklist.h
 *
 * @Author  Frank Wang
 * @Date    2010-10-06
 */

#ifndef _MTC_LINKLIST_H_
#define _MTC_LINKLIST_H_

#include <stdlib.h>

#include "mtc_types.h"

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

/**
 * list_for_each    -    iterate over a list
 * @pos:    the &struct list_head to use as a loop cursor.
 * @head:   the head for your list.
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop cursor.
 * @n:      another &struct list_head to use as temporary storage
 * @head:   the head for your list.
 */
 #define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

/**
 * list_for_each_i    -    iterate over a list
 * @pos:    the &struct list_head to use as a loop cursor.
 * @head:   the head for your list.
 */
#define list_for_each_i(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_i_safe - iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop cursor.
 * @n:      another &struct list_head to use as temporary storage
 * @head:   the head for your list.
 */
 #define list_for_each_i_safe(pos, n, head) \
    for (pos = (head)->prev, n = pos->prev; pos != (head); \
        pos = n, n = pos->prev)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({  \
    (type *)( (char *)ptr - ((size_t) &((type *)0)->member) );})

/**
 * list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
            container_of(ptr, type, member)

unsigned char list_empty(const struct list_head *head);
void list_add(struct list_head *new, struct list_head *prev, struct list_head *next);
void list_add_tail(struct list_head *new, struct list_head *head);
void list_del(struct list_head *me);
void list_replace(struct list_head *old, struct list_head *new);
void list_move(struct list_head *list, struct list_head *head);

#define list_add_head(new, head) \
            list_add((new), (head), (head)->next)
#define list_init(list) \
            (list)->next = (list)->prev = list

#endif

