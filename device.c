#include "device.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static FILE *f;

void device_new_disk(const char *path)
{
    //printf("%s\n", __FUNCTION__);
    f=fopen(path, "w+");
    
    uint32_t empty_blocks[BLOCK_SIZE];
    int i;
    for(i=1; i<BLOCK_SIZE; i++)
	{
       empty_blocks[i]=0xFFFFFFFF;
	}
    empty_blocks[0]=0xFFFFFFE0;

    unsigned char *char_map=(unsigned char*)calloc(1, BLOCK_SIZE*sizeof(uint32_t));
    unsigned char *char_map_start=char_map;
    memcpy(&char_map[0], empty_blocks, BLOCK_SIZE*sizeof(uint32_t));

    //Bits map
    device_write_block(char_map, 0);
    char_map+=1023;
    device_write_block(char_map, 1);
    char_map+=1023;
	device_write_block(char_map, 2);
    char_map+=1023;
    device_write_block(char_map, 3);
    
    char_map=char_map_start;
    free(char_map);

    //Root
    struct Directory root;
    strcpy(root.name, "root");

    unsigned char *char_root=(unsigned char*)calloc(1, sizeof(root));
    memcpy(&char_root[0], &root, sizeof(root));
    device_write_block(char_root, 4);

    free(char_root);
}

void device_open(const char *path) 
{
    //printf("%s\n", __FUNCTION__);
    f=fopen(path, "r+");
	
    if(f==NULL)
    {
        device_new_disk(path);
    }
}

void device_close()
{
    fflush(f);
    fclose(f);
}

int device_read_block(unsigned char buffer[], int block) 
{
    fseek(f, block*BLOCK_SIZE, SEEK_SET);
	
    return (fread(buffer, 1, BLOCK_SIZE, f)==BLOCK_SIZE);
}

int device_write_block(unsigned char buffer[], int block) 
{
    fseek(f, block*BLOCK_SIZE, SEEK_SET);
	
    return (fwrite(buffer, 1, BLOCK_SIZE, f)==BLOCK_SIZE);
}

void device_format()
{
    unsigned char empty_block[BLOCK_SIZE];
    int i;
    for(i=0; i<BLOCK_SIZE; i++)
	{
       empty_block[i]=0;
	}

    //Bits map
    device_write_block(empty_block, 0);
    device_write_block(empty_block, 1);
	device_write_block(empty_block, 2);
    device_write_block(empty_block, 3);

    //Root directory
    device_write_block(empty_block, 4);
}

void device_flush()
{
    fflush(f);
}
