/* What the game needs from the calculator (plat_eadk.c), or from a computer for the tests. */
#ifndef JJ_PLAT_H
#define JJ_PLAT_H
#include <stdbool.h>
#include <stdint.h>

enum { JK_OK = 1, JK_UP = 2, JK_DOWN = 4, JK_BACK = 8, JK_HOME = 16, JK_ANY = 32 };
uint32_t plat_keys(void);
uint32_t plat_millis(void);
void plat_sleep(uint32_t ms);
bool plat_save(const char *name, const void *data, uint32_t len);
const uint8_t *plat_load(const char *name, uint32_t *len);
uint32_t plat_random(void);
void plat_begin(void);
int plat_end(void);
#endif
