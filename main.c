#include "filesystem.h"
#include <stdio.h>

static struct fuse_operations operations={
    .init=filesystem_init,
    .getattr=filesystem_getattr,
    .mkdir=filesystem_mkdir,
    .readdir=filesystem_readdir,
    .mknod=filesystem_mknod
};

int main(int argc, char *argv[])
{
    device_open(argv[1]);

    int i=1;
    for(; i < argc; i++)
    {
		argv[i] = argv[i+1];
	}
	argc--;

	int fuse_stat=fuse_main(argc, argv, &operations, NULL);

    device_close();

	return fuse_stat;
}