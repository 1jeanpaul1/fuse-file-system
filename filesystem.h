#ifndef filesystem_h
#define filesystem_h

#include "device.h"
#include <fuse.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DIRECTORY_NAME 32
#define ENTRY_SIZE sizeof(struct Directory_entry)
#define MAX_FILE_NAME 23

#define MAX_BLOCKS_PER_FILE BLOCK_SIZE/sizeof(int)
#define MAX_FILE_SIZE MAX_BLOCKS_PER_FILE*BLOCK_SIZE
#define MAX_DIRECTORY_ENTRIES (BLOCK_SIZE-MAX_DIRECTORY_NAME)/ENTRY_SIZE

struct Directory_entry
{
    char name[MAX_FILE_NAME];
    int index_block;
    char isDir;
};

struct Directory 
{
    char name[MAX_DIRECTORY_NAME];
    struct Directory_entry entries[MAX_DIRECTORY_ENTRIES];
};

void filesystem_set_bit(int n, int value);
int filesystem_get_free_block();

void filesystem_load_map();
void filesystem_update_map();
void filesystem_load_root();
void filesystem_update_root();
struct Directory* filesystem_load_directory(int n);

void filesystem_init(struct fuse_conn_info *conn);
struct Directory_entry *filesystem_get_entry(const char *name);
int filesystem_getattr(const char *path, struct stat *statbuf);
int filesystem_mkdir(const char *path, mode_t mode);
int filesystem_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int filesystem_mknod(const char *path, mode_t mode, dev_t dev);

#ifdef __cplusplus
}
#endif

#endif //filesystem_h