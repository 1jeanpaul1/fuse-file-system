#include "device.h"
#include <stdio.h>

static FILE *f;

void device_new_disk(const char *path)
{
    f=fopen(path, "w+");
    
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

void device_open(const char *path) 
{
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
