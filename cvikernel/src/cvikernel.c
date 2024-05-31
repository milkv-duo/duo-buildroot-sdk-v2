#include "kernel_internal.h"
#include "cvikernel/cvikernel.h"

typedef struct internal_data {
  ec_t ec;
  mode_manager_t mode_manager;
  uint32_t cmdbuf_ptr;
  uint32_t max_nr_desc;
  uint32_t cur_nr_desc;
  desc_pair_t *desc_pairs;
  uint32_t lmem_ptr;
} internal_data_t;

/* Avoid to export interface */
extern char *cvikernel_get_chip_info_1822(void);
extern void cvikernel_init_1822(
    cvk_reg_info_t *req_info,
    cvk_context_t *context);
#if CHIPID == 0x3
extern char *cvikernel_get_chip_info_cv181x(void);
extern void cvikernel_init_cv181x(
    cvk_reg_info_t *req_info,
    cvk_context_t *context);
#elif CHIPID == 0x4
extern char *cvikernel_get_chip_info_cv180x(void);
extern void cvikernel_init_cv180x(
    cvk_reg_info_t *req_info,
    cvk_context_t *context);
#elif CHIPID == 0x1
extern char *cvikernel_get_chip_info_1880v2(void);
extern void cvikernel_init_1880v2(
    cvk_reg_info_t *req_info,
    cvk_context_t *context);
#elif CHIPID == 0x2
#else
extern char *cvikernel_get_chip_info_cv181x(void);
extern void cvikernel_init_cv181x(
    cvk_reg_info_t *req_info,
    cvk_context_t *context);
extern char *cvikernel_get_chip_info_cv180x(void);
extern void cvikernel_init_cv180x(
    cvk_reg_info_t *req_info,
    cvk_context_t *context);
extern char *cvikernel_get_chip_info_1880v2(void);
extern void cvikernel_init_1880v2(
    cvk_reg_info_t *req_info,
    cvk_context_t *context);
#endif

typedef struct chip_query_info {
  char *(*get_chip_version)(void);
  void (*chip_init)(cvk_reg_info_t *req_info, cvk_context_t *context);
} chip_query_info_t;

// Supported chips
static chip_query_info_t cvikernel_chip_list[] = {
#if CHIPID == 0x3
  {cvikernel_get_chip_info_cv181x, cvikernel_init_cv181x},
#elif CHIPID == 0x4
  {cvikernel_get_chip_info_cv180x, cvikernel_init_cv180x},
#elif CHIPID == 0x1
  {cvikernel_get_chip_info_1880v2, cvikernel_init_1880v2},
#elif CHIPID == 0x2
#else
  {cvikernel_get_chip_info_cv181x, cvikernel_init_cv181x},
  {cvikernel_get_chip_info_cv180x, cvikernel_init_cv180x},
  {cvikernel_get_chip_info_1880v2, cvikernel_init_1880v2},
#endif
  {cvikernel_get_chip_info_1822, cvikernel_init_1822}
};

#define NUM_DEVICES (sizeof(cvikernel_chip_list)/sizeof(chip_query_info_t))

cvk_context_t *cvikernel_register(cvk_reg_info_t *req_info)
{
  if (!req_info)
    return NULL;
  if (!req_info->cmdbuf)
    return NULL;

  size_t req_chip_size = sizeof(req_info->chip_ver_str);
  size_t req_chip_len = strlen(req_info->chip_ver_str);

  for (size_t i = 0; i < NUM_DEVICES; i++) {
    char *version = (*cvikernel_chip_list[i].get_chip_version)();

    // Compare chip string
    if (!strncmp(version, req_info->chip_ver_str, req_chip_size) &&
        strlen(version) == req_chip_len) {
      cvk_context_t *context = malloc(sizeof(cvk_context_t));
      if (!context)
        return NULL;

      (*cvikernel_chip_list[i].chip_init)(req_info, context);
      return context;
    }
  }

  return NULL;
}
