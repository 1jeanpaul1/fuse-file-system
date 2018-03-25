#include "device.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static FILE *f;

void device_new_disk(const char *path, int device_size)
{
    if(debug) printf("%s\n", __FUNCTION__);

    f=fopen(path, "w+");
    
    uint32_t empty_blocks[BLOCK_SIZE];
    int i;
    for(i=1; i<BLOCK_SIZE; i++)
	{
       empty_blocks[i]=0xFFFFFFFF;
	}
    empty_blocks[0]=0xFFFFFFC0;

    unsigned char *char_map=(unsigned char*)calloc(1, BLOCK_SIZE*sizeof(uint32_t));
    unsigned char *char_map_start=char_map;
    memcpy(&char_map[0], empty_blocks, BLOCK_SIZE*sizeof(uint32_t));

    device_write_block(char_map, 0);
    char_map+=4096;
    device_write_block(char_map, 1);
    char_map+=4096;
	device_write_block(char_map, 2);
    char_map+=4096;
    device_write_block(char_map, 3);
    
    char_map=char_map_start;
    free(char_map);

    struct Directory root;

    for(i=0; i<MAX_DIRECTORY_ENTRIES; i++)
    {
        root.entries[i].index_block=0;
    }

    unsigned char *char_root=(unsigned char*)calloc(1, sizeof(root));
    memcpy(&char_root[0], &root, sizeof(root));
    device_write_block(char_root, 4);

    free(char_root);

    if(device_size<=0)
    {
        device_size=BLOCK_SIZE*4*8*BLOCK_SIZE;
    }

    unsigned char *char_device_size=(unsigned char*)calloc(1, BLOCK_SIZE);
    memcpy(char_device_size, &device_size, sizeof(device_size));
    device_write_block(char_device_size, 5);

    free(char_device_size);
}

void device_open(const char *path, int device_size)
{
    if(debug) printf("%s\n", __FUNCTION__);

    f=fopen(path, "r+");
	
    if(f==NULL)
    {
        device_new_disk(path, device_size);
    }
}

void device_close()
{
    if(debug) printf("%s\n", __FUNCTION__);

    fflush(f);
    fclose(f);
}

int device_read_block(unsigned char buffer[], int block) 
{
    if(debug) printf("%s\n", __FUNCTION__);

    fseek(f, block*BLOCK_SIZE, SEEK_SET);
	
    return (fread(buffer, 1, BLOCK_SIZE, f)==BLOCK_SIZE);
}

int device_write_block(unsigned char buffer[], int block) 
{
    if(debug) printf("%s\n", __FUNCTION__);

    fseek(f, block*BLOCK_SIZE, SEEK_SET);
	
    return (fwrite(buffer, 1, BLOCK_SIZE, f)==BLOCK_SIZE);
}

void device_format()
{
    if(debug) printf("%s\n", __FUNCTION__);

    uint32_t empty_blocks[BLOCK_SIZE];
    int i;
    for(i=1; i<BLOCK_SIZE; i++)
	{
       empty_blocks[i]=0xFFFFFFFF;
	}
    empty_blocks[0]=0xFFFFFFC0;

    unsigned char *char_map=(unsigned char*)calloc(1, BLOCK_SIZE*sizeof(uint32_t));
    unsigned char *char_map_start=char_map;
    memcpy(&char_map[0], empty_blocks, BLOCK_SIZE*sizeof(uint32_t));

    device_write_block(char_map, 0);
    char_map+=4096;
    device_write_block(char_map, 1);
    char_map+=4096;
	device_write_block(char_map, 2);
    char_map+=4096;
    device_write_block(char_map, 3);
    
    char_map=char_map_start;
    free(char_map);

    struct Directory root;
  
    for(i=0; i<MAX_DIRECTORY_ENTRIES; i++)
    {
        root.entries[i].index_block=0;
    }

    unsigned char *char_root=(unsigned char*)calloc(1, sizeof(root));
    memcpy(&char_root[0], &root, sizeof(root));
    device_write_block(char_root, 4);

    free(char_root);
}

void device_flush()
{
    if(debug) printf("%s\n", __FUNCTION__);
    
    fflush(f);
}
