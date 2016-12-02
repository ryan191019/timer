
/**
 * @file mtc_linklist.c Linking List help function
 *
 * @author  Frank Wang
 * @date    2010-10-06
 */

#include "include/mtc_linklist.h"

/**
 *  check list is empty or not
 *
 *  @param *head    list head
 *
 *  @return True for empty and False for other else.
 */
unsigned char list_empty(const struct list_head *head)
{
    return head->next == head;
}

/**
 *  add a node after prev and before next
 *
 *  @param *new     new node
 *  @param *prev    prev node
 *  @param *next    next node
 *
 *  @return none
 */
void inline list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/**
 *  add a node at tail
 *
 *  @param *new     new node
 *  @param *head    head node
 *
 *  @return none
 */
void inline list_add_tail(struct list_head *new, struct list_head *head)
{
    list_add(new, head->prev, head);
}

/**
 *  remove a node from list
 *
 *  @param *me       the node that will be remove
 *
 *  @return none
 */
void inline list_del(struct list_head *me)
{
    me->prev->next = me->next;
    me->next->prev = me->prev;
}

/**
 *  replace a node from list
 *
 *  @param *old     the old node that will be remove from list
 *  @param *new     the new node that will be add into list
 *
 *  @return none
 */
void inline list_replace(struct list_head *old, struct list_head *new)
{
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
void inline list_move(struct list_head *list, struct list_head *head)
{
    list_del(list);
    list_add_tail(list, head);
}


