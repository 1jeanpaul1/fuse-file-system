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

void filesystem_load_root()
{
    printf("%s\n", __FUNCTION__);
    
    unsigned char *char_root=(unsigned char*)calloc(1, sizeof(root));
    device_read_block(char_root, 4);
    memcpy(&root, &char_root[0], sizeof(root));
    free(char_root);
}

void filesystem_update_root()
{
    printf("%s\n", __FUNCTION__);
    
    unsigned char *char_root=(unsigned char*)calloc(1, sizeof(root));
    memcpy(&char_root[0], &root, sizeof(root));
    device_write_block(char_root, 4);
    free(char_root);
}

struct Directory* filesystem_load_directory(int n)
{
    printf("%s\n", __FUNCTION__);
    
    unsigned char *char_directory=(unsigned char*)calloc(1, sizeof(struct Directory));
    device_read_block(char_directory, n);

    struct Directory *directory=(struct Directory *)calloc(1, sizeof(*directory));

    memcpy(directory, &char_directory[0], sizeof(*directory));
    free(char_directory);
    
    return directory;
}

void filesystem_free_blocks(struct Directory_entry *entry)
{
    unsigned char *char_index=(unsigned char*)calloc(1, sizeof(struct Index_block));
    device_read_block(char_index, entry->index_block);

    struct Index_block *index_block=(struct Index_block *)calloc(1, sizeof(struct Index_block));

    memcpy(index_block, &char_index[0], sizeof(*index_block));
    free(char_index);

    int i;
    for(i=0; i<MAX_BLOCKS_PER_FILE; i++)
    {
        if(index_block->blocks[i]!=0)
        {
            filesystem_set_bit(index_block->blocks[i], 1);
        }
    }
    filesystem_set_bit(entry->index_block, 1);

    free(index_block);

    filesystem_update_map();
}

void* filesystem_init(struct fuse_conn_info *conn) 
{
    printf("%s\n", __FUNCTION__);
    filesystem_load_map();
    filesystem_load_root();

    return NULL;
}

struct Directory_entry *filesystem_get_entry(const char *name)
{
    printf("%s\n", __FUNCTION__);

    int i;
    int in_root=1;
    for(i=1; i<MAX_DIRECTORY_NAME; i++)
    {
        if(name[i]=='/')
        {
            in_root=0;
            break;
        }
    }

    struct Directory *directory;
    if(in_root)
    {
        directory=&root;
        i=0;
    }
    else
    {
        char directory_name[MAX_DIRECTORY_NAME];
        memset(directory_name, '\0', MAX_DIRECTORY_NAME);

        memcpy(&directory_name, &name[0], i);
        struct Directory_entry* entry=filesystem_get_entry(&directory_name[0]);

        if(entry==NULL)
        {
            return NULL;
        }

        directory=filesystem_load_directory(entry->index_block);
    }

    char file_name[MAX_DIRECTORY_NAME];
    memset(file_name, '\0', MAX_DIRECTORY_NAME);    
    strcpy(file_name, &name[i+1]);

    i=0;
    while(i<MAX_DIRECTORY_ENTRIES)
    {    
        if(directory->entries[i].index_block!=0)
        {
            if(strcmp(file_name, directory->entries[i].name)==0)
            {
                return &directory->entries[i];
            }
        }   
        i++;
    }
    return NULL;
}

void filesystem_get_file_size(struct Directory_entry* entry, int *size, int *blocks)
{
    *blocks=0;
    *size=0;

    unsigned char *char_index=(unsigned char*)calloc(1, sizeof(struct Index_block));
    device_read_block(char_index, entry->index_block);

    struct Index_block *index_block=(struct Index_block *)calloc(1, sizeof(struct Index_block));

    memcpy(index_block, &char_index[0], sizeof(*index_block));
    free(char_index);

    int i;
    for(i=0; index_block->blocks[i]!=0; i++)
    {
        (*blocks)++;
    }

    for(i=0; i<(*blocks)-1; i++)
    {
        (*size)+=BLOCK_SIZE;
    }

    char *block_info=(char *)calloc(1, BLOCK_SIZE);
    device_read_block((unsigned char*) block_info, index_block->blocks[i]);
    (*size)+=strlen(block_info);

    free(block_info);
    free(index_block);
}

int filesystem_getattr(const char *path, struct stat *statbuf)
{
    printf("%s\n", __FUNCTION__);
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
        struct Directory_entry* entry=filesystem_get_entry(&path[0]);
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
            int size, blocks;
            filesystem_get_file_size(entry, &size, &blocks);

            statbuf->st_mode=S_IFREG | 0777;
            statbuf->st_nlink=1;
            statbuf->st_ino=0;
            statbuf->st_uid=0;
            statbuf->st_gid=0;
            statbuf->st_size=size; 
            statbuf->st_blksize=BLOCK_SIZE;
            statbuf->st_blocks=blocks;
        }
    }
    return 0;
}

int filesystem_mkdir(const char *path, mode_t mode) 
{
    printf("%s\n", __FUNCTION__);
    
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

    int j;
    for(j=0; j<MAX_DIRECTORY_ENTRIES; j++)
    {
        directory.entries[j].index_block=0;
    }

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
    printf("%s\n", __FUNCTION__);
    
    struct Directory directory;
    if(strcmp(path, "/")== 0)
	{
        directory=root;
	}
    else
    {
        struct Directory_entry* entry=filesystem_get_entry(&path[0]);

        if(entry==NULL)
        {
            return -ENOENT;
        }

        directory=*(filesystem_load_directory(entry->index_block));
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

int filesystem_mknod(const char *path, mode_t mode, dev_t dev)
{
    printf("%s\n", __FUNCTION__);

    if( S_ISREG(mode)) 
    {
        struct Directory_entry* entry=filesystem_get_entry(&path[0]);

        if(entry!=NULL)
        {
            return -EEXIST;
        }

        int i;
        int in_root=1;
        for(i=1; i<MAX_DIRECTORY_NAME; i++)
        {
            if(path[i]=='/')
            {
                in_root=0;
                break;
            }
        }

        if(in_root)
        {
            i=0;
        }

        int block;
        struct Directory *directory;
        if(in_root)
        {
            directory=&root;
            block=4;
        }
        else
        {
            char name[MAX_DIRECTORY_NAME];
            memset(name, '\0', MAX_DIRECTORY_NAME);
            memcpy(&name, &path[0], i);
            entry=filesystem_get_entry(&name[0]);

            if(entry==NULL)
            {
                return -ENOENT;
            }

            block=entry->index_block;
            directory=filesystem_load_directory(block);
        }

        int j=0;
        while(j<MAX_DIRECTORY_ENTRIES)
        {    
            if(directory->entries[j].index_block==0)
            {
                break;
            }   
            j++;
        }

        if(directory->entries[j].index_block!=0)
        {
            return -ENOSPC;
        }

        strcpy(directory->entries[j].name, &path[i+1]);
        directory->entries[j].isDir=0;
        directory->entries[j].index_block=filesystem_get_free_block();
        filesystem_set_bit(directory->entries[j].index_block, 0);
        filesystem_update_map();

        struct Index_block index_block;
        
        int x;
        for(x=0; x<MAX_BLOCKS_PER_FILE; x++)
        {
            index_block.blocks[x]=0;
        }


        unsigned char *char_directory=(unsigned char*)calloc(1, sizeof(*directory));
        memcpy(&char_directory[0], directory, sizeof(*directory));
        device_write_block(char_directory, block);
        free(char_directory);

        unsigned char *char_index=(unsigned char*)calloc(1, sizeof(index_block));
        memcpy(&char_index[0], &index_block, sizeof(index_block));
        device_write_block(char_index, directory->entries[j].index_block);
        free(char_index);

        return 0;
    }
    return -EPERM;
}

int filesystem_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    printf("%s\n", __FUNCTION__);

    struct Directory_entry* entry=filesystem_get_entry(&path[0]);
   
    if(entry==NULL)
    {
        return -ENOENT;
    }

    unsigned char *char_index=(unsigned char*)calloc(1, sizeof(struct Index_block));
    device_read_block(char_index, entry->index_block);

    struct Index_block *index_block=(struct Index_block *)calloc(1, sizeof(struct Index_block));

    memcpy(index_block, &char_index[0], sizeof(*index_block));
    free(char_index);
    int new_block=0;

    int i;
    for(i=0; i<MAX_BLOCKS_PER_FILE; i++)
    {
        if(index_block->blocks[i]==0)
        {
            break;
        }
    }
    //TO DO

    if(i!=0)
    {
        char *block_info=(char *)calloc(1, BLOCK_SIZE);
        device_read_block((unsigned char*) block_info, index_block->blocks[i-1]);

        if(strlen(block_info)<BLOCK_SIZE)
        {
            i--;
        }
        else
        {
            new_block=filesystem_get_free_block();
            filesystem_set_bit(new_block, 0);
            index_block->blocks[i]=new_block;
            filesystem_update_map();

            char_index=(unsigned char*)calloc(1, sizeof(*index_block));
            memcpy(&char_index[0], index_block, sizeof(*index_block));
            device_write_block(char_index, entry->index_block);
            free(char_index);
        }
    }
    else
    {
        new_block=filesystem_get_free_block();
        filesystem_set_bit(new_block, 0);
        index_block->blocks[i]=new_block;
        filesystem_update_map();

        char_index=(unsigned char*)calloc(1, sizeof(*index_block));
        memcpy(&char_index[0], index_block, sizeof(*index_block));
        device_write_block(char_index, entry->index_block);
        free(char_index);
    }

    unsigned char *char_info=(unsigned char*)calloc(1, BLOCK_SIZE);
        
    if(!new_block)
    {
        device_read_block(char_info, index_block->blocks[i]);
    }

    if((int)offset>=(BLOCK_SIZE*i))
    {
        offset=(off_t)(((int)offset)-(BLOCK_SIZE*i));
    }

    memcpy(&char_info[offset], buf, size);
    device_write_block(char_info,  index_block->blocks[i]);

    free(char_info);
    free(index_block);
    return size;
}

int filesystem_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    printf("%s\n", __FUNCTION__);

    struct Directory_entry* entry=filesystem_get_entry(&path[0]);

    if(entry==NULL)
    {
        return -ENOENT;
    }

    char *info=(char *)calloc(1, MAX_FILE_SIZE);

    unsigned char *char_index=(unsigned char*)calloc(1, sizeof(struct Index_block));
    device_read_block(char_index, entry->index_block);

    struct Index_block *index_block=(struct Index_block *)calloc(1, sizeof(struct Index_block));

    memcpy(index_block, &char_index[0], sizeof(*index_block));
    free(char_index);

    int x;
    for(x=0; x<MAX_BLOCKS_PER_FILE; x++)
    {
        if(index_block->blocks[x]!=0)
        {
            char *block_info=(char *)calloc(1, BLOCK_SIZE);
            device_read_block((unsigned char*) block_info, index_block->blocks[x]);

            strcat(info, block_info);
            free(block_info);
        }
    }
    int info_size=strlen(info);
    memcpy(buf, &info[0], info_size);

    free(info);
    free(index_block);

    return info_size;
}

int filesystem_rename(const char *path, const char *newpath)
{
    printf("%s\n", __FUNCTION__);

    int i;
    int in_root=1;
    for(i=1; i<MAX_DIRECTORY_NAME; i++)
    {
        if(path[i]=='/')
        {
            in_root=0;
            break;
        }
    }

    if(in_root)
    {
        struct Directory_entry* entry=filesystem_get_entry(&path[0]);

        if(entry==NULL)
        {
            return -ENOENT;
        }

        strcpy(entry->name, &newpath[1]);

        filesystem_update_root();
        return 0;
    }
    else
    {
        char directory_name[MAX_DIRECTORY_NAME];
        memset(directory_name, '\0', MAX_DIRECTORY_NAME);
        memcpy(&directory_name, &path[0], i);
        struct Directory_entry* entry=filesystem_get_entry(&directory_name[0]);

        if(entry==NULL)
        {
            return -ENOENT;;
        }

        struct Directory *directory=filesystem_load_directory(entry->index_block);

        char file_name[MAX_DIRECTORY_NAME];
        memset(file_name, '\0', MAX_DIRECTORY_NAME);
        strcpy(file_name, &path[i+1]);

        int name_start=i+1;

        i=0;
        while(i<MAX_DIRECTORY_ENTRIES)
        {    
            if(directory->entries[i].index_block!=0)
            {
                if(strcmp(file_name, directory->entries[i].name)==0)
                {
                    strcpy(directory->entries[i].name, &newpath[name_start]);

                    unsigned char *char_directory=(unsigned char*)calloc(1, sizeof(*directory));
                    memcpy(&char_directory[0], directory, sizeof(*directory));
                    device_write_block(char_directory, entry->index_block);
                    free(char_directory);

                    return 0;
                }
            }   
            i++;
        }
        return -ENOENT;
    }
    return -ENOENT;
}

int filesystem_unlink(const char *path)
{
    printf("%s\n", __FUNCTION__);

    int i;
    int in_root=1;
    for(i=1; i<MAX_DIRECTORY_NAME; i++)
    {
        if(path[i]=='/')
        {
            in_root=0;
            break;
        }
    }

    if(in_root)
    {
        struct Directory_entry* entry=filesystem_get_entry(&path[0]);

        if(entry==NULL)
        {
            return -ENOENT;
        }

        filesystem_free_blocks(entry);
        entry->index_block=0;

        filesystem_update_root();
        return 0;
    }
    else
    {
        char directory_name[MAX_DIRECTORY_NAME];
        memset(directory_name, '\0', MAX_DIRECTORY_NAME);
        memcpy(&directory_name, &path[0], i);
        struct Directory_entry* entry=filesystem_get_entry(&directory_name[0]);

        if(entry==NULL)
        {
            return -ENOENT;;
        }

        struct Directory *directory=filesystem_load_directory(entry->index_block);

        char file_name[MAX_DIRECTORY_NAME];
        memset(file_name, '\0', MAX_DIRECTORY_NAME);
        strcpy(file_name, &path[i+1]);

        i=0;
        while(i<MAX_DIRECTORY_ENTRIES)
        {    
            if(directory->entries[i].index_block!=0)
            {
                if(strcmp(file_name, directory->entries[i].name)==0)
                {
                    filesystem_free_blocks(&directory->entries[i]);
                    directory->entries[i].index_block=0;

                    unsigned char *char_directory=(unsigned char*)calloc(1, sizeof(*directory));
                    memcpy(&char_directory[0], directory, sizeof(*directory));
                    device_write_block(char_directory, entry->index_block);
                    free(char_directory);

                    return 0;
                }
            }   
            i++;
        }
        return -ENOENT;
    }
    return -ENOENT;
}

int filesystem_rmdir(const char *path) 
{
    printf("%s\n", __FUNCTION__);

    struct Directory_entry* entry=filesystem_get_entry(&path[0]);

    if(entry==NULL)
    {
        return -ENOENT;;
    }

    struct Directory *directory=filesystem_load_directory(entry->index_block);

    int i;
    for(i=0; i<MAX_DIRECTORY_ENTRIES; i++)
    {
        if(directory->entries[i].index_block!=0)
        {
            filesystem_free_blocks(&directory->entries[i]);
        }
    }

    filesystem_set_bit(entry->index_block, 1);
    entry->index_block=0;

    filesystem_update_root();
    filesystem_update_map();

    return 0;
}