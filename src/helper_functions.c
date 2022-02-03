
#include "helper_functions.h"

/**
    @brief Turn integer into a char array
    @param n integer to convert into string
    @param s string to return
*/
void itoa(int n, char s[])
 {
     int i, sign;
 
     if ((sign = n) < 0)
         n = -n;          
     i = 0;
     do {       
         s[i++] = n % 10 + '0';   
     } while ((n /= 10) > 0); 
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

/**
    @brief Reverses the string
    @param s string to reverse
*/
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/**
    Takes a 32 bit unsigned integer and sets a specific
    bit given
    @param map bitmap whoses bit to set
    @param n position of the bit
*/
uint32_t set_bit(uint32_t map, int n){
    return map | (1 << BITMAP_OFFSET(n));
}

/** 
*    @brief Sets the desired bit of the bitmap back to 0
*    @param map bitmap whose bit to clear
*    @param n position of the bit
*/
uint32_t clear_bit(uint32_t map, int n){
    return map & ~(1 << BITMAP_OFFSET(n));
}

/**
    
    @brief Checks if the first n bits of the bitmap are set.

    @param map bitmap whose bits to validate
    @param n amount of bits to check
*/
bool full_set(uint32_t map, int n){
    int i = 0;
    for(;i < n; i++){
        if(!(map & (1 << i))) return false;
    }
    return true;
}