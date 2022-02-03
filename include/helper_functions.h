#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H
#include <stdint.h>
#include <string.h>
#include "globals.h"


#define BITMAP_OFFSET(u) (u%32)
void itoa(int n, char s[]);
void reverse(char s[]);
uint32_t set_bit(uint32_t map, int n);
uint32_t clear_bit(uint32_t map, int n);
bool full_set(uint32_t map, int n);

#endif // !HELPER_FUNCTIONS_H