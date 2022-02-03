/* Copied from https://www.journaldev.com/35238/hash-table-in-c-plus-plus */
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "llist.h"

#define CAPACITY 50000 // Size of the Hash Table

typedef struct Ht_item Ht_item;
 
// Define the Hash Table Item here
struct Ht_item {
    char* key;
    bool value;
};

typedef struct HashTable HashTable;
 
// Define the Hash Table here
struct HashTable {
    // Contains an array of pointers
    // to items
    Ht_item** items;
    int size;
    int count;
};

unsigned long hash_function(char* str);
Ht_item* create_item(char* key, bool value);
HashTable* create_table(int size);
void free_item(Ht_item* item);
void free_table(HashTable* table);
void ht_insert(HashTable* table, char* key, bool value);
bool ht_search(HashTable* table, char* key);
void print_search(HashTable* table, char* key);
void print_table(HashTable* table);

extern HashTable* archive;

#endif // !HASHTABLE_H