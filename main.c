#include "filesystem.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    device_open(argv[1]);
    filesystem_load_map();
    printf("%d\n", filesystem_get_free_block());
    filesystem_set_bit(5, 0);
    filesystem_set_bit(6, 0);
    filesystem_set_bit(7, 0);
    filesystem_set_bit(8, 0);
    filesystem_update_map();
    printf("%d\n", filesystem_get_free_block());
    filesystem_set_bit(3, 1);
    printf("%d\n", filesystem_get_free_block());
}