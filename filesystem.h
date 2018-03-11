#ifndef filesystem_h
#define filesystem_h

#ifdef __cplusplus
extern "C" {
#endif

void filesystem_init_bitmap();
void filesystem_set_bit(int n, int value);
int filesystem_get_free_block();

#ifdef __cplusplus
}
#endif

#endif //filesystem_h