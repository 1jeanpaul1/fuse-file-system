#include "filesystem.h"
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
    debug=atoi(argv[3]);

    device_open(argv[1], atoi(argv[4]));

    if(atoi(argv[2]))
    {
        device_format();
    }

    int i;
    if(atoi(argv[4])!=0)
    {
        for(i=1; i<argc; i++)
        {
            argv[i]=argv[i+1];
        }
        argc--;
    }

    for(i=1; i<argc; i++)
    {
		argv[i] = argv[i+1];
	}
	argc--;

    for(i=1; i<argc; i++)
    {
		argv[i]=argv[i+1];
	}
	argc--;

    for(i=1; i<argc; i++)
    {
		argv[i]=argv[i+1];
	}
	argc--;

 	int fuse_stat=fuse_main(argc, argv, &operations, NULL);

    device_close();

	return fuse_stat;

    //TO DO
    //free
}