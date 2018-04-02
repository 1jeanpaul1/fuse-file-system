#include "filesystem.h"
#include <math.h>

#define BITS_PER_WORD 32
#define WORD_OFFSET(n) ((n)/BITS_PER_WORD)
#define BIT_OFFSET(n) ((n)%BITS_PER_WORD)
#define CHECK_BIT(word, n) (((word)>>(n))&1)

uint32_t map[BLOCK_SIZE];
struct Directory root;
int debug;
int device_size;

void filesystem_set_bit(int n, int value)
{
    if(debug) printf("%s\n", __FUNCTION__);

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
    if(debug) printf("%s\n", __FUNCTION__);

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

    if(!CHECK_BIT(map[i], j) || (BITS_PER_WORD*i+j)>=floor(device_size/BLOCK_SIZE))
    {
        return -1;
    }
    return BITS_PER_WORD*i+j;
}

int filesystem_count_free_blocks()
{
    if(debug) printf("%s\n", __FUNCTION__);
    
    int count=0;
    int i;
    for(i=0; i<BLOCK_SIZE; i++)
    {
        int j;
        for(j=0; j<BITS_PER_WORD; j++)
        {
            if((BITS_PER_WORD*i+j)>=floor(device_size/BLOCK_SIZE))
            {
                i=BLOCK_SIZE;
                break;
            }
            if(CHECK_BIT(map[i], j))
            {
                count++;
            }
        }
    }

    return count;
}

void filesystem_load_map()
{
    if(debug) printf("%s\n", __FUNCTION__);

    unsigned char *char_map=(unsigned char*)malloc(BLOCK_SIZE*sizeof(uint32_t));
    unsigned char *char_map_start=char_map;
    device_read_block(char_map, 0);
    char_map+=4096;
    device_read_block(char_map, 1);
    char_map+=4096;    
    device_read_block(char_map, 2);
    char_map+=4096;
    device_read_block(char_map, 3);
    
    char_map=char_map_start;
    memcpy(map, char_map, BLOCK_SIZE*sizeof(uint32_t));
    free(char_map);
}

void filesystem_update_map()
{
    if(debug) printf("%s\n", __FUNCTION__);

    unsigned char *char_map=(unsigned char*)malloc(BLOCK_SIZE*sizeof(uint32_t));
    unsigned char *char_map_start=char_map;
    memcpy(char_map, map, BLOCK_SIZE*sizeof(uint32_t));

    device_write_block(char_map, 0);
    char_map+=4096;
    device_write_block(char_map, 1);
    char_map+=4096;
	device_write_block(char_map, 2);
    char_map+=4096;
    device_write_block(char_map, 3);

    char_map=char_map_start;
    free(char_map);
}

void filesystem_load_device_size()
{
    unsigned char *char_device_size=(unsigned char*)malloc(BLOCK_SIZE);
    device_read_block(char_device_size, 5);
    memcpy(&device_size, char_device_size, sizeof(device_size));

    free(char_device_size);
}

void filesystem_load_root()
{
    if(debug) printf("%s\n", __FUNCTION__);
    
    unsigned char *char_root=(unsigned char*)malloc(sizeof(root));
    device_read_block(char_root, 4);
    memcpy(&root, char_root, sizeof(root));
    free(char_root);
}

void filesystem_update_root()
{
    if(debug) printf("%s\n", __FUNCTION__);
    
    unsigned char *char_root=(unsigned char*)malloc(sizeof(root));
    memcpy(char_root, &root, sizeof(root));
    device_write_block(char_root, 4);
    free(char_root);
}

struct Directory* filesystem_load_directory(int n)
{
    if(debug) printf("%s\n", __FUNCTION__);
    
    struct Directory *directory=(struct Directory *)malloc(sizeof(struct Directory));
    
    unsigned char *char_directory=(unsigned char*)malloc(sizeof(*directory));
    device_read_block(char_directory, n);

    memcpy(directory, char_directory, sizeof(*directory));
    free(char_directory);
    
    return directory;
}

void filesystem_free_blocks(struct Directory_entry *entry)
{
    if(debug) printf("%s\n", __FUNCTION__);

    struct Index_block *index_block=(struct Index_block *)malloc(sizeof(struct Index_block));
    
    unsigned char *char_index=(unsigned char*)malloc(sizeof(*index_block));
    device_read_block(char_index, entry->index_block);

    memcpy(index_block, char_index, sizeof(*index_block));
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
    if(debug) printf("%s\n", __FUNCTION__);

    filesystem_load_map();
    filesystem_load_root();
    filesystem_load_device_size();

    return NULL;
}

struct Directory_entry *filesystem_get_entry(const char *name)
{
    if(debug) printf("%s\n", __FUNCTION__);

    int i;
    int in_root=1;
    for(i=1; i<MAX_FILE_NAME+1; i++)
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
        char directory_name[MAX_FILE_NAME+1];
        memset(directory_name, '\0', MAX_FILE_NAME+1);

        memcpy(&directory_name, name, i);
        struct Directory_entry* entry=filesystem_get_entry(directory_name);

        if(entry==NULL)
        {
            return NULL;
        }

        directory=filesystem_load_directory(entry->index_block);
    }

    char file_name[MAX_FILE_NAME];
    memset(file_name, '\0', MAX_FILE_NAME);    
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
    if(debug) printf("%s\n", __FUNCTION__);

    *blocks=0;
    *size=0;

    struct Index_block *index_block=(struct Index_block *)malloc(sizeof(struct Index_block));
    
    unsigned char *char_index=(unsigned char*)malloc(sizeof(*index_block));
    device_read_block(char_index, entry->index_block);

    memcpy(index_block, char_index, sizeof(*index_block));
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

    if((*blocks)>0) 
    {
        char *block_info=(char *)malloc(BLOCK_SIZE);
        device_read_block((unsigned char*) block_info, index_block->blocks[i]);

        int my_strlen=BLOCK_SIZE;
        for(i=BLOCK_SIZE-1; i>=0; i--)
        {
            if(block_info[i]!=MY_NULL)
            {
                break;
            }
            my_strlen--;
        }
        (*size)+=my_strlen;

        free(block_info);
    }
    free(index_block);
}

int filesystem_getattr(const char *path, struct stat *statbuf)
{
    if(debug) printf("%s\n", __FUNCTION__);

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
        struct Directory_entry* entry=filesystem_get_entry(path);
        if(entry==NULL)
        {
            return -ENOENT;
        }
        
        if(entry->is_dir)
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
            int size, blocks;
            filesystem_get_file_size(entry, &size, &blocks);

            statbuf->st_mode=S_IFREG|0777;
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
    if(debug) printf("%s\n", __FUNCTION__);

    if(strlen(&path[1])>=MAX_FILE_NAME)
    {
        return -ENAMETOOLONG;
    }

    int i;
    for(i=1; i<(int)strlen(path); i++)
    {
        if(path[i]=='/')
        {
            return -EPERM;
        }
    }
    
    i=0;
    while(i<MAX_DIRECTORY_ENTRIES)
    {    
        if(root.entries[i].index_block==0)
        {
            break;
        }   
        i++;
    }

    int free_block=filesystem_get_free_block();

    if(i>=MAX_DIRECTORY_ENTRIES || free_block==-1)
    {
        return -ENOSPC;
    }

    strcpy(root.entries[i].name, &path[1]);
    root.entries[i].is_dir=1;
    root.entries[i].index_block=free_block;
    filesystem_set_bit(root.entries[i].index_block, 0);

    struct Directory directory;

    int j;
    for(j=0; j<MAX_DIRECTORY_ENTRIES; j++)
    {
        directory.entries[j].index_block=0;
    }

    unsigned char *char_directory=(unsigned char*)malloc(sizeof(directory));
    memcpy(char_directory, &directory, sizeof(directory));
    device_write_block(char_directory, root.entries[i].index_block);

    free(char_directory);

    filesystem_update_root();
    filesystem_update_map();

    return 0;
}

int filesystem_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    if(debug) printf("%s\n", __FUNCTION__);
    
    struct Directory directory;
    if(strcmp(path, "/")==0)
	{
        directory=root;
	}
    else
    {
        struct Directory_entry* entry=filesystem_get_entry(path);

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
    if(debug) printf("%s\n", __FUNCTION__);

    if(S_ISREG(mode)) 
    {
        struct Directory_entry* entry=filesystem_get_entry(path);

        if(entry!=NULL)
        {
            return -EEXIST;
        }

        int i;
        int in_root=1;
        for(i=1; i<MAX_FILE_NAME+1; i++)
        {
            if(path[i]=='/')
            {
                in_root=0;
                break;
            }
        }

        int block;
        struct Directory *directory;
        if(in_root)
        {
            directory=&root;
            i=0;
            block=4;
        }
        else
        {
            char directory_name[MAX_FILE_NAME+1];
            memset(directory_name, '\0', MAX_FILE_NAME+1);
            memcpy(&directory_name, path, i);
            entry=filesystem_get_entry(directory_name);

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

        int free_block=filesystem_get_free_block();

        if(j>=MAX_DIRECTORY_ENTRIES || free_block==-1)
        {
            return -ENOSPC;
        }

        if(strlen(&path[i+1])>=MAX_FILE_NAME)
        {
            return -ENAMETOOLONG;
        }

        strcpy(directory->entries[j].name, &path[i+1]);
        directory->entries[j].is_dir=0;
        directory->entries[j].index_block=free_block;
        filesystem_set_bit(directory->entries[j].index_block, 0);
        filesystem_update_map();

        struct Index_block index_block;
        
        int x;
        for(x=0; x<MAX_BLOCKS_PER_FILE; x++)
        {
            index_block.blocks[x]=0;
        }

        unsigned char *char_directory=(unsigned char*)malloc(sizeof(*directory));
        memcpy(char_directory, directory, sizeof(*directory));
        device_write_block(char_directory, block);
        free(char_directory);

        unsigned char *char_index=(unsigned char*)malloc(sizeof(index_block));
        memcpy(char_index, &index_block, sizeof(index_block));
        device_write_block(char_index, directory->entries[j].index_block);
        free(char_index);

        return 0;
    }
    return -EPERM;
}

int filesystem_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    if(debug) printf("%s\n", __FUNCTION__);

    struct Directory_entry* entry=filesystem_get_entry(path);
   
    if(entry==NULL)
    {
        return -ENOENT;
    }

    struct Index_block *index_block=(struct Index_block *)malloc(sizeof(struct Index_block));
    
    unsigned char *char_index=(unsigned char*)malloc(sizeof(*index_block));
    device_read_block(char_index, entry->index_block);

    memcpy(index_block, char_index, sizeof(*index_block));
    free(char_index);


    int start_block=floor(((double)offset)/BLOCK_SIZE);
    int blocks_to_write=ceil(((double)size)/BLOCK_SIZE);
    int x;

    for(x=0; x<blocks_to_write; x++)
    {
        if((x+start_block)>=MAX_BLOCKS_PER_FILE)
        {
            return -EFBIG;
        }
        if(index_block->blocks[x+start_block]!=0)
        {
            unsigned char *block_info=(unsigned char*)malloc(BLOCK_SIZE);
            memset(block_info, MY_NULL, BLOCK_SIZE);
            
            device_read_block(block_info, index_block->blocks[x+start_block]);

            int my_offset=(int)offset;
            if(my_offset>=(BLOCK_SIZE*(x+start_block)))
            {
                my_offset-=(BLOCK_SIZE*(x+start_block));
            }

            memcpy(&block_info[my_offset], buf, size);
            device_write_block(block_info,  index_block->blocks[x+start_block]);

            free(block_info);
        }
        else
        {
            int new_block=filesystem_get_free_block();

            if(new_block==-1)
            {
                return -ENOSPC;
            }

            filesystem_set_bit(new_block, 0);
            filesystem_update_map();
            index_block->blocks[x+start_block]=new_block;

            unsigned char *char_index=(unsigned char*)malloc(sizeof(*index_block));
            memcpy(char_index, index_block, sizeof(*index_block));
            device_write_block(char_index, entry->index_block);
            free(char_index);

            unsigned char *block_info=(unsigned char *)malloc(BLOCK_SIZE);
            memset(block_info, MY_NULL, BLOCK_SIZE);

            memcpy(block_info, buf, size);
            device_write_block(block_info,  index_block->blocks[x+start_block]);

            free(block_info);
        }
    }

    free(index_block);
    return size;
}

int filesystem_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    if(debug) printf("%s\n", __FUNCTION__);

    struct Directory_entry* entry=filesystem_get_entry(path);

    if(entry==NULL)
    {
        return -ENOENT;
    }

    unsigned char *info=(unsigned char*)calloc(1, (int)size);

    struct Index_block *index_block=(struct Index_block *)malloc(sizeof(struct Index_block));
    
    unsigned char *char_index=(unsigned char*)malloc(sizeof(*index_block));
    device_read_block(char_index, entry->index_block);

    memcpy(index_block, char_index, sizeof(*index_block));
    free(char_index);

    int start_block=floor(((double)offset)/BLOCK_SIZE);
    int blocks_to_read=ceil(((double)size)/BLOCK_SIZE);
    int x;

    int my_offset=0;
    for(x=0; x<blocks_to_read; x++)
    {
        if((x+start_block)>=MAX_BLOCKS_PER_FILE)
        {
            return -ESPIPE;
        }
        if(index_block->blocks[x+start_block]!=0)
        {
            unsigned char *block_info=(unsigned char*)malloc(BLOCK_SIZE);
            device_read_block(block_info, index_block->blocks[x+start_block]);

            memcpy(&info[my_offset], block_info, BLOCK_SIZE);

            my_offset+=BLOCK_SIZE;

            free(block_info);
        }
    }
    
    memcpy(buf, info, size);

    free(info);
    free(index_block);

    return size;
}

int filesystem_rename(const char *path, const char *newpath)
{
    if(debug) printf("%s\n", __FUNCTION__);

    filesystem_unlink(newpath);

    int i;
    int in_root=1;
    for(i=1; i<MAX_FILE_NAME+1; i++)
    {
        if(path[i]=='/')
        {
            in_root=0;
            break;
        }
    }

    if(in_root)
    {
        if(strlen(&newpath[1])>=MAX_FILE_NAME)
        {
            return -ENAMETOOLONG;
        }
        
        struct Directory_entry* entry=filesystem_get_entry(path);

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
        if(strlen(&newpath[i+1])>=MAX_FILE_NAME)
        {
            return -ENAMETOOLONG;
        }

        char directory_name[MAX_FILE_NAME+1];
        memset(directory_name, '\0', MAX_FILE_NAME+1);
        memcpy(&directory_name, path, i);

        struct Directory_entry* entry=filesystem_get_entry(directory_name);

        if(entry==NULL)
        {
            return -ENOENT;;
        }

        struct Directory *directory=filesystem_load_directory(entry->index_block);

        char file_name[MAX_FILE_NAME];
        memset(file_name, '\0', MAX_FILE_NAME);
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

                    unsigned char *char_directory=(unsigned char*)malloc(sizeof(*directory));
                    memcpy(char_directory, directory, sizeof(*directory));
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
    if(debug) printf("%s\n", __FUNCTION__);

    int i;
    int in_root=1;
    for(i=1; i<MAX_FILE_NAME+1; i++)
    {
        if(path[i]=='/')
        {
            in_root=0;
            break;
        }
    }

    if(in_root)
    {
        struct Directory_entry* entry=filesystem_get_entry(path);

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
        char directory_name[MAX_FILE_NAME+1];
        memset(directory_name, '\0', MAX_FILE_NAME+1);
        memcpy(&directory_name, path, i);

        struct Directory_entry* entry=filesystem_get_entry(directory_name);

        if(entry==NULL)
        {
            return -ENOENT;
        }

        struct Directory *directory=filesystem_load_directory(entry->index_block);

        char file_name[MAX_FILE_NAME];
        memset(file_name, '\0', MAX_FILE_NAME);
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

                    unsigned char *char_directory=(unsigned char*)malloc(sizeof(*directory));
                    memcpy(char_directory, directory, sizeof(*directory));
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
    if(debug) printf("%s\n", __FUNCTION__);

    struct Directory_entry* entry=filesystem_get_entry(path);

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

int filesystem_statfs(const char *path, struct statvfs *statInfo) 
{
	if(debug) printf("%s\n", __FUNCTION__);
        
    statInfo->f_bsize=BLOCK_SIZE;

    int free_blocks=filesystem_count_free_blocks();

    statInfo->f_bfree=free_blocks;
    statInfo->f_bavail=free_blocks;

    statInfo->f_namemax=MAX_FILE_NAME-1;
    
    return 0;
}
