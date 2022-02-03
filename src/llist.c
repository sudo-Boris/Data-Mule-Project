#include "CC1200_lib.h"


llist *llist_create(char *new_data)
{
    node *new_node;

    llist *new_list = (llist *)malloc(sizeof(llist));
    *new_list = (node *)malloc(sizeof(node));
    
    new_node = *new_list;
    new_node->data = new_data;
    new_node->next = NULL;
    return new_list;
}

void llist_free(llist *list)
{
    node *curr = *list;
    node *next;

    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    free(list);
}

void llist_push(llist *list, char *data)
{
    node *head;
    node *new_node;
    if (list == NULL || *list == NULL) {
        fprintf(stderr, "llist_add_inorder: list is null\n");
    }

    head = *list;
    
    // Head is empty node
    if (head->data == NULL){
        head->data = (char*) malloc(strlen(data) + 1);
        strcpy(head->data, data);

    // Head is not empty, add new node to front
    }else{
        new_node = malloc(sizeof(node));
        new_node->data = (char*) malloc(strlen(data) + 1);
        strcpy(new_node->data, data);
        new_node->next = head;
        *list = new_node;
    }
}

char *llist_pop(llist *list)
{

    char *popped_data;
    node *head = *list;

    if (list == NULL || !head || head->data == NULL)
        return NULL;
    
    popped_data = head->data;
    *list = head->next;
    free(head);

    return popped_data;
}

bool llist_find(llist* list, char* data){
    node* head = *list;
    if(!list || !head || head->data == NULL)
        return false;
    while(head && head->data != NULL){
        if(!strcmp(head->data, data))
            return true;
        head = head->next;
    }
    return false;
}

void llist_update_flag(llist* list, char* data, bool flag){
    node* head = *list;
    if(!list || !head || head->data == NULL)
        return;
     while(head->data != NULL){
        if(!strcmp(head->data, data)){
            head->checked = flag;
            return;
        }
        head = head->next;
    }

}

void llist_delete_data(llist* list, char* data){
    node* head = *list;
    if(!list || !head || head->data == NULL)
        return;
    node* prev = head;
    if(!strcmp(head->data, data)){
        llist_pop(list);
        return;
    } 
    while(head->data != NULL){
        if(!strcmp(head->data, data)){
            prev->next = head->next;
            free(head->data);
            free(head);
            break; 
        }
        prev = head;    
        head = head->next;
        
    }
}



void llist_print(llist *list)
{
    node *curr = *list;
    while (curr != NULL) {
        printf("%s -- ",curr->data);
        curr = curr->next;
    }
    putchar('\n');
}

