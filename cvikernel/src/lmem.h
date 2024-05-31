#include "kernel_internal.h"

#define LMEM_MAX_BANKS      8
#define BANK_ALL_ID LMEM_MAX_BANKS

typedef struct {
  uint32_t size;
  uint32_t ptr;
  uint32_t eu_num;
} bank_t;

typedef struct {
  bmk_chip_info_t *chip_info;
  bank_t banks[LMEM_MAX_BANKS + 1];
} lmem_t;

void lmem_init(lmem_t *lmem, bmk_chip_info_t *chip_info);
uint32_t lmem_alloc(
    lmem_t *lmem, int bank_id, int size, fmt_t fmt,
    uint8_t eu_align, uint8_t la_align);
void lmem_free(lmem_t *lmem, int bank_id, uint32_t la, int size);
