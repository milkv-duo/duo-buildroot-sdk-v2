#include "lmem.h"

static uint32_t align_la_bm1880(uint32_t la, fmt_t fmt, uint32_t eu_num)
{
  int data_size = bitsize_of_fmt(fmt) / 8;
  ASSERT(data_size == 1 || data_size == 2);
  la = ceiling_func(la, data_size);
  la = ALIGN(la, eu_num) * data_size;
  return la;
}

static void bank_init(bank_t *b, uint32_t size, uint32_t eu_num)
{
  b->size = size;
  b->ptr = 0;
  b->eu_num = eu_num;
}

static uint32_t bank_alloc_bm1880(
    bank_t *b, int size, fmt_t fmt, uint8_t eu_align, uint8_t la_align)
{
  uint32_t la = b->ptr;
  if (eu_align || la_align)
    la = align_la_bm1880(la, fmt, b->eu_num);

  if (la + size > b->size)
    return -1;

  b->ptr = la + size;
  return la;
}

static void bank_free(bank_t *b, uint32_t la, uint32_t size)
{
  ASSERT(size <= b->ptr);
  ASSERT(la <= b->ptr);
  ASSERT(la + size <= b->ptr);

  b->ptr = la;
}

static void lmem_mark_bank_used(lmem_t *lmem, int bank_id)
{
  uint32_t bank_base_addr = bank_id * lmem->chip_info->lmem_bank_size;
  bank_t *b = &lmem->banks[BANK_ALL_ID];
  ASSERT(b->ptr <= bank_base_addr);
  if (b->size > bank_base_addr)
    b->size = bank_base_addr;
}

static void lmem_mark_bank_free(lmem_t *lmem, uint32_t bank_id)
{
  uint32_t bank_size = lmem->chip_info->lmem_bank_size;
  bank_t *all_bank = &lmem->banks[BANK_ALL_ID];

  if (bank_id != all_bank->size / bank_size)
    return;

  for (int i = bank_id; i < LMEM_MAX_BANKS; i++) {
    if (lmem->banks[i].ptr == 0)
      all_bank->size += bank_size;
    else
      break;
  }
}

void lmem_init(lmem_t *lmem, bmk_chip_info_t *chip_info)
{
  uint32_t eu_num = chip_info->eu_num;
  uint32_t lmem_size = chip_info->lmem_size;
  uint32_t bank_size = chip_info->lmem_bank_size;
  uint32_t lmem_banks = chip_info->lmem_banks;
  ASSERT(lmem_banks <= LMEM_MAX_BANKS);

  lmem->chip_info = chip_info;
  bank_init(&lmem->banks[BANK_ALL_ID], lmem_size, eu_num);
  for (uint32_t i = 0; i < lmem_banks; i++)
    bank_init(&lmem->banks[i], bank_size, eu_num);
}

uint32_t lmem_alloc(
    lmem_t *lmem, int bank_id, int size, fmt_t fmt,
    uint8_t eu_align, uint8_t la_align)
{
  uint32_t chip_version = lmem->chip_info->version;
  bank_t *b = &lmem->banks[bank_id];

  uint32_t la = -1;
  if (chip_version == 1880)
    la = bank_alloc_bm1880(b, size, fmt, eu_align, la_align);

  if (la == (uint32_t)-1)
    return -1;

  if (bank_id != BANK_ALL_ID)
    lmem_mark_bank_used(lmem, bank_id);

  return la;
}

void lmem_free(lmem_t *lmem, int bank_id, uint32_t la, int size)
{
  uint32_t bank_size = lmem->chip_info->lmem_bank_size;

  uint32_t bank_base_addr = 0;
  if (bank_id != BANK_ALL_ID)
    bank_base_addr = bank_id * bank_size;

  bank_t *b = &lmem->banks[bank_id];
  bank_free(b, la - bank_base_addr, size);

  if (bank_id != BANK_ALL_ID)
    if (b->ptr == 0)
      lmem_mark_bank_free(lmem, bank_id);
}
