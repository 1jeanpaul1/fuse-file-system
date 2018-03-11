#include "filesystem.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    filesystem_init_bitmap();
    printf("%d\n", filesystem_get_free_block());
    filesystem_set_bit(0, 0);
    filesystem_set_bit(1, 0);
    filesystem_set_bit(2, 0);
    filesystem_set_bit(3, 0);
    printf("%d\n", filesystem_get_free_block());
    filesystem_set_bit(3, 1);
    printf("%d\n", filesystem_get_free_block());
}