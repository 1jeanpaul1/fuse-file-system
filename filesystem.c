#include "filesystem.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define BITS_PER_WORD 32
#define WORD_OFFSET(n) ((n)/BITS_PER_WORD)
#define BIT_OFFSET(n) ((n)%BITS_PER_WORD)
#define CHECK_BIT(word, n) (((word)>>(n))&1)

uint32_t map[BLOCK_SIZE];

void filesystem_set_bit(int n, int value)
{
    printf("%s\n", __FUNCTION__);
    if(value)
    {
        map[WORD_OFFSET(n)]|=(1<<BIT_OFFSET(n));
    } 
    else 
    {
        map[WORD_OFFSET(n)]&=~(1<<BIT_OFFSET(n));  
    }
}

int filesystem_get_free_block()
{
    printf("%s\n", __FUNCTION__);
    int i=0;
    while(map[i]==0 && i<BLOCK_SIZE)
    {
        i++;
    }

    int j=0, k=BITS_PER_WORD;
    while(!CHECK_BIT(map[i], j) && j<BITS_PER_WORD)
    {
        j++;
        k--;
    }

    return BITS_PER_WORD*i+j;
}

void filesystem_load_map()
{
     printf("%s\n", __FUNCTION__);
    unsigned char *char_map=(unsigned char*)calloc(1, BLOCK_SIZE*sizeof(uint32_t));
    unsigned char *char_map_start=char_map;
    device_read_block(char_map, 0);
    char_map+=1023;
    device_read_block(char_map, 1);
    char_map+=1023;    
    device_read_block(char_map, 2);
    char_map+=1023;
    device_read_block(char_map, 3);
    char_map=char_map_start;
    memcpy(map, &char_map[0], BLOCK_SIZE*sizeof(uint32_t));
    free(char_map);
}

void filesystem_update_map()
{
    printf("%s\n", __FUNCTION__);
    unsigned char *char_map=(unsigned char*)calloc(1, BLOCK_SIZE*sizeof(uint32_t));
    unsigned char *char_map_start=char_map;
    memcpy(&char_map[0], map, BLOCK_SIZE*sizeof(uint32_t));

    device_write_block(char_map, 0);
    char_map+=1023;
    device_write_block(char_map, 1);
    char_map+=1023;
	device_write_block(char_map, 2);
    char_map+=1023;
    device_write_block(char_map, 3);

    char_map=char_map_start;
    free(char_map);
}
