#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

static struct fuse_operations operations={
    .init=filesystem_init,
    .getattr=filesystem_getattr,
    .mkdir=filesystem_mkdir,
    .readdir=filesystem_readdir,
    .mknod=filesystem_mknod,
    .write=filesystem_write,
    .read=filesystem_read,
    .rename=filesystem_rename,
    .unlink=filesystem_unlink,
    .rmdir=filesystem_rmdir,
    .statfs=filesystem_statfs
};

int main(int argc, char *argv[])
{
    device_open(argv[1]);

    int i;
    for(i=1; i < argc; i++)
    {
		argv[i] = argv[i+1];
	}
	argc--;

    if(atoi(argv[1]))
    {
        device_format();
    }

    for(i=1; i < argc; i++)
    {
		argv[i] = argv[i+1];
	}
	argc--;

 	int fuse_stat=fuse_main(argc, argv, &operations, NULL);

    device_close();

	return fuse_stat;

    //TO DO
    //free
}