#ifndef filesystem_h
#define filesystem_h

#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DIRECTORY_NAME 32
#define ENTRY_SIZE 32
#define MAX_FILE_NAME 27

#define MAX_BLOCKS_PER_FILE BLOCK_SIZE/sizeof(int)
#define MAX_FILE_SIZE MAX_BLOCKS_PER_FILE*BLOCK_SIZE
#define MAX_DIRECTORY_ENTRIES BLOCK_SIZE/ENTRY_SIZE-1

struct directory_entry
{
    char name[MAX_FILE_NAME];
    int index_block;
    char isDir;
};

struct directory 
{
    char name[MAX_DIRECTORY_NAME];
    struct directory_entry entries[MAX_DIRECTORY_ENTRIES];
};

void filesystem_init_bitmap();
void filesystem_set_bit(int n, int value);
int filesystem_get_free_block();

#ifdef __cplusplus
}
#endif

#endif //filesystem_h