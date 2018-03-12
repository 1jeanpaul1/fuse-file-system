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
struct Directory root;

void filesystem_set_bit(int n, int value)
{
    //printf("%s\n", __FUNCTION__);
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
    //printf("%s\n", __FUNCTION__);
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
     //printf("%s\n", __FUNCTION__);
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
    //printf("%s\n", __FUNCTION__);
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

void filesystem_load_root()
{
    //printf("%s\n", __FUNCTION__);
    
    unsigned char *char_root=(unsigned char*)calloc(1, sizeof(root));
    device_read_block(char_root, 4);
    memcpy(&root, &char_root[0], sizeof(root));
    free(char_root);
}

void filesystem_update_root()
{
    //printf("%s\n", __FUNCTION__);
    
    unsigned char *char_root=(unsigned char*)calloc(1, sizeof(root));
    memcpy(&char_root[0], &root, sizeof(root));
    device_write_block(char_root, 4);
    free(char_root);
}

struct Directory* filesystem_load_directory(int n)
{
    //printf("%s\n", __FUNCTION__);
    
    unsigned char *char_directory=(unsigned char*)calloc(1, sizeof(struct Directory));
    device_read_block(char_directory, n);

    struct Directory *directory=(struct Directory *)char_directory;
    
    return directory;
}

void filesystem_init(struct fuse_conn_info *conn) 
{
    //printf("%s\n", __FUNCTION__);
    filesystem_load_map();
    filesystem_load_root();
}

struct Directory_entry *filesystem_get_entry(const char *name)
{
    //printf("%s\n", __FUNCTION__);
    int i=0;
    while(i<MAX_DIRECTORY_ENTRIES)
    {    
        if(root.entries[i].index_block!=0)
        {
            if(strncmp(name, root.entries[i].name, MAX_FILE_NAME)==0)
            {
                return &root.entries[i];
            }
        }   
        i++;
    }
    return NULL;
}

int filesystem_getattr(const char *path, struct stat *statbuf)
{
    //printf("%s\n", __FUNCTION__);
    if (strcmp(path, "/")==0)
    {
        statbuf->st_mode=S_IFDIR|0777;
		statbuf->st_uid=0;
        statbuf->st_gid=0;
        statbuf->st_nlink=1;
        statbuf->st_ino=0;
        statbuf->st_size=BLOCK_SIZE;
        statbuf->st_blksize=BLOCK_SIZE;
        statbuf->st_blocks=1;
    }
    else
    {
        struct Directory_entry* entry=filesystem_get_entry(&path[1]);
        if(entry==NULL)
        {
            return -ENOENT;
        }
        
        if(entry->isDir)
        {
            statbuf->st_mode=S_IFDIR | 0777;
            statbuf->st_uid=0;
            statbuf->st_gid=0;
            statbuf->st_nlink=1;
            statbuf->st_ino=0;
            statbuf->st_size=BLOCK_SIZE;
            statbuf->st_blksize=BLOCK_SIZE;
            statbuf->st_blocks=1;
        }
        else
        {
            //is file
        }
    }
    return 0;
}

int filesystem_mkdir(const char *path, mode_t mode) 
{
    //printf("%s\n", __FUNCTION__);
    
    int i=0;
    while(i<MAX_DIRECTORY_ENTRIES)
    {    
        if(root.entries[i].index_block==0)
        {
            break;
        }   
        i++;
    }

    strcpy(root.entries[i].name, &path[1]);
    root.entries[i].isDir=1;
    root.entries[i].index_block=filesystem_get_free_block();
    filesystem_set_bit(root.entries[i].index_block, 0);

    struct Directory directory;
    strcpy(directory.name, root.entries[i].name);

    unsigned char *char_directory=(unsigned char*)calloc(1, sizeof(directory));
    memcpy(&char_directory[0], &directory, sizeof(directory));
    device_write_block(char_directory, root.entries[i].index_block);

    free(char_directory);

    filesystem_update_root();
    filesystem_update_map();

    return 0;
}

int filesystem_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    //printf("%s\n", __FUNCTION__);
    
    struct Directory directory;
    if(strcmp(path, "/")== 0)
	{
        directory=root;
	}
    else
    {
        struct Directory_entry* entry=filesystem_get_entry(&path[1]);

        if(entry==NULL)
        {
            return -ENOENT;
        }

        directory=*(filesystem_load_directory(entry->index_block));
        //printf("\n%s\n", directory.name);
    }

    int i=0;
    while(i<MAX_DIRECTORY_ENTRIES)
    {    
        if(directory.entries[i].index_block!=0)
        {
            if(filler(buffer, directory.entries[i].name, NULL, 0)!=0)
            {
                return -ENOMEM;
            }
        }   
        i++;
    }

    return 0;
}