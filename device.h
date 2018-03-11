#ifndef device_h
#define device_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE 4096

void device_new_device(const char *path);
void device_open(const char *path);
void device_close();
int device_read_block(unsigned char buffer[], int block);
int device_write_block(unsigned char buffer[], int block);
void device_format();
void device_flush();

#ifdef __cplusplus
}
#endif

#endif //device_h