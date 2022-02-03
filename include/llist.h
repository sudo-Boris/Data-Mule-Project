/* Copied from https://gist.github.com/meylingtaing/11018042 */

#ifndef LLIST_H
#define LLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hyperparameters.h"

typedef struct _node {
   char *data;
   bool checked;
   struct _node *next;
}node;

typedef struct _node * llist;

/* llist_create: Create a linked list */
llist *llist_create(char *data);

/* llist_free: Free a linked list */
void llist_free(llist *list);

/* llist_push: Add to head of list */
void llist_push(llist *list, char *data);

/* llist_pop: remove and return head of linked list */
char *llist_pop(llist *list);

/* llist_find: look if data is in list */
bool llist_find(llist* list, char* data);

/* llist_delete_data: find data and delete it */
void llist_delete_data(llist* list, char* data);

/* llist_print: print linked list */
void llist_print(llist *list);

/* llist_update_flag: find data in list and update flag */
void llist_update_flag(llist* list, char* data, bool flag);

#endif // !LLIST_H