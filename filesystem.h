#ifndef filesystem_h
#define filesystem_h

#include "device.h"
#include <fuse.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG 0

#define MY_NULL 'r'

#define MAX_DIRECTORY_NAME (int)32
#define ENTRY_SIZE (int)sizeof(struct Directory_entry)
#define MAX_FILE_NAME (int)24

#define MAX_BLOCKS_PER_FILE (int)(BLOCK_SIZE/sizeof(int))
#define MAX_FILE_SIZE (int)(MAX_BLOCKS_PER_FILE*BLOCK_SIZE)
#define MAX_DIRECTORY_ENTRIES (int)((BLOCK_SIZE-MAX_DIRECTORY_NAME)/ENTRY_SIZE)

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

struct Index_block
{
    int blocks[MAX_BLOCKS_PER_FILE];
};

void filesystem_set_bit(int n, int value);
int filesystem_get_free_block();

void filesystem_load_map();
void filesystem_update_map();
void filesystem_load_root();
void filesystem_update_root();
struct Directory* filesystem_load_directory(int n);
void filesystem_free_blocks(struct Directory_entry *entry);

void* filesystem_init(struct fuse_conn_info *conn);
struct Directory_entry *filesystem_get_entry(const char *name);
void filesystem_get_file_size(struct Directory_entry*, int *size, int *blocks);
int filesystem_getattr(const char *path, struct stat *statbuf);
int filesystem_mkdir(const char *path, mode_t mode);
int filesystem_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int filesystem_mknod(const char *path, mode_t mode, dev_t dev);
int filesystem_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
int filesystem_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) ;
int filesystem_rename(const char *path, const char *newpath);
int filesystem_unlink(const char *path);
int filesystem_rmdir(const char *path);

int filesystem_statfs(const char *path, struct statvfs *statInfo);

#ifdef __cplusplus
}
#endif

#endif //filesystem_h