#include "hashTable.h"
 
unsigned long hash_function(char* str) {
    unsigned long i = 0;
    int j=0;
    for (; str[j]; j++)
        i += str[j];
    return i % CAPACITY;
}
 

 

 
Ht_item* create_item(char* key, bool value) {
    // Creates a pointer to a new hash table item
    Ht_item* item = (Ht_item*) malloc (sizeof(Ht_item));
    item->key = (char*) malloc (strlen(key) + 1);
    item->value = value;
    
    strcpy(item->key, key);
 
    return item;
}
 
HashTable* create_table(int size) {
    // Creates a new HashTable
    HashTable* table = (HashTable*) malloc (sizeof(HashTable));
    table->size = size;
    table->count = 0;
    table->items = (Ht_item**) calloc (table->size, sizeof(Ht_item*));
    int i=0;
    for (; i<table->size; i++)
        table->items[i] = NULL;
 
    return table;
}
 
void free_item(Ht_item* item) {
    // Frees an item
    free(item->key);
    free(item);
}
 
void free_table(HashTable* table) {
    // Frees the table
    int i=0;
    for (; i<table->size; i++) {
        Ht_item* item = table->items[i];
        if (item != NULL)
            free_item(item);
    }
 
    free(table->items);
    free(table);
}
 
void handle_collision(HashTable* table, unsigned long index, Ht_item* item) {
}
 
void ht_insert(HashTable* table, char* key, bool value) {
    // Create the item
    Ht_item* item = create_item(key, value);
 
    // Compute the index
    unsigned long index = hash_function(key);
 
    Ht_item* current_item = table->items[index];
     
    if (current_item == NULL) {
        // Key does not exist.
        if (table->count == table->size) {
            // Hash Table Full
            printf("Insert Error: Hash Table is full\n");
            // Remove the create item
            free_item(item);
            return;
        }
         
        // Insert directly
        table->items[index] = item; 
        table->count++;
    }
 
    else {
            // Scenario 1: We only need to update value
            if (strcmp(current_item->key, key) == 0) {
                table->items[index]->value = value;
                return;
            }
     
        else {
            // Scenario 2: Collision
            // We will handle case this a bit later
            handle_collision(table, index, item);
            return;
        }
    }
}
 
bool ht_search(HashTable* table, char* key) {
    // Searches the key in the hashtable
    // and returns NULL if it doesn't exist
    int index = hash_function(key);
    Ht_item* item = table->items[index];
 
    // Ensure that we move to a non NULL item
    if (item != NULL) {
        if (strcmp(item->key, key) == 0)
            return item->value;
    }
    return false;
}
 
void print_search(HashTable* table, char* key) {
    bool val;
    if ((val = ht_search(table, key)) == false) {
        printf("Key:%s does not exist\n", key);
        return;
    }
    else {
        printf("Key:%s, Value:%s\n", key, val);
    }
}

void print_table(HashTable* table) {
    printf("\nHash Table\n-------------------\n");
    int i=0;
    for (; i<table->size; i++) {
        if (table->items[i]) {
            printf("Index:%d, Key:%s, Value:%s\n", i, table->items[i]->key, table->items[i]->value);
        }
    }
    printf("-------------------\n\n");
}