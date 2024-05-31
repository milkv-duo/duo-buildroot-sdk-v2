#ifndef BMDNN_MMPOOL_H_
#define BMDNN_MMPOOL_H_

#include <bmkernel/bm_kernel.h>

#include <utility>
#include <vector>
#include <iostream>
#include <map>
#include <algorithm>
#include <stdio.h>
#include <assert.h>

#ifdef __linux__
#define POOL_USE_PTHREAD
#include <pthread.h>
#endif

#include <stdint.h>

#define GLOBAL_MEM_ADDR_NULL    (0x000000ffffffffffULL)

using namespace std;

//#define MEM_POOL_NAIVE
//#define MEM_POOL_ZEPHRE
#define MEM_POOL_NAIVE_PLUS

#define MEM_POOL_ADDR_INVALID   (GLOBAL_MEM_ADDR_NULL)
#define MEM_POOL_SLOT_NUM       (2048 * 8)
#define BANK_SIZE               (0x80000000)

#ifdef MEM_POOL_NAIVE
#endif

#ifdef MEM_POOL_ZEPHRE
typedef u64 pool_addr_t;
struct pool_quad_block {
    pool_addr_t mem_blocks;
    u32 mem_status;
};

struct pool_block_set {
    int block_size;
    int nr_of_entries;
    struct pool_quad_block *quad_block;
#ifdef CONFIG_OBJECT_MONITOR
    int count;
#endif
};

struct pool_struct {
    int maxblock_size;
    int minblock_size;
    int nr_of_maxblocks;
    int nr_of_block_sets;
    struct pool_block_set *block_set;
    pool_addr_t bufblock;
};
#define MAX_POOL_COUNT          (2)
#endif /* MEM_POOL_ZEPHRE */

#ifdef MEM_POOL_NAIVE_PLUS
#define MIN_SLOT_SIZE (4 * 1024)

#define CHECK_FREED_MEM(offset, size, P)                                      \
  do {                                                                        \
    pool_map_t::iterator it;                                                  \
    for (it = P->slot_in_use.begin(); it != P->slot_in_use.end(); it++) {     \
      if( ((*it).first <= offset && (*it).first + (*it).second > offset) ||   \
          ((*it).first < offset + size && (*it).first + (*it).second >= offset + size) ){ \
        printf("CHECK_FREED_MEM ERROR: attempting to free memory in use\n");  \
        assert(0);                                                            \
      }                                                                       \
    }                                                                         \
  } while(0);

typedef u64 pool_addr_t;
typedef u64 pool_size_t;
typedef pair<pool_addr_t, pool_size_t> pool_pair_t;
typedef map<pool_addr_t, pool_size_t> pool_map_t;

class offset_prev_finder {
public:
  offset_prev_finder(pool_addr_t offset_prev) : offset_prev_(offset_prev) {}
  bool operator () (const pool_pair_t &pair_){
    return pair_.first + pair_.second == offset_prev_;
  }
private:
  pool_addr_t offset_prev_;
};

class offset_next_finder {
public:
  offset_next_finder(pool_addr_t offset_next) : offset_next_(offset_next) {}
  bool operator () (const pool_pair_t &pair_){
    return pair_.first == offset_next_;
  }
private:
  pool_addr_t offset_next_;
};

struct pool_struct {
  int num_slots_in_use;
  vector<pool_pair_t> slot_avail;  // (offset, size)
  pool_map_t slot_in_use; // offset -> size
};
#define MAX_POOL_COUNT (2)

#endif /* MEM_POOL_NAIVE_PLUS */

typedef struct mem_pool {
  u64                           total_size;
#ifdef MEM_POOL_NAIVE
  u64                           head_addr;
  u64                           slot[MEM_POOL_SLOT_NUM];
  int                           head_slot;
  int                           slot_used;
#endif /* MEM_POOL_NAIVE */
#ifdef MEM_POOL_ZEPHRE
  struct pool_struct            _k_mem_pool_list[MAX_POOL_COUNT];
  int                           _k_mem_pool_count;
  /* managing allocated chunk */
  u64                           slot_addr[MEM_POOL_SLOT_NUM];
  int                           slot_size[MEM_POOL_SLOT_NUM];
  int                           slot_used;
#endif /* MEM_POOL_ZEPHRE */
#ifdef MEM_POOL_NAIVE_PLUS
  struct pool_struct            _mem_pool_list[MAX_POOL_COUNT];
  int                           _mem_pool_count;
  /* managing allocated chunk */
  u64                           slot_addr[MEM_POOL_SLOT_NUM];
  int                           slot_size[MEM_POOL_SLOT_NUM];
  int                           slot_used;
#endif /* MEM_POOL_ZEPHRE */
#ifdef POOL_USE_PTHREAD
  pthread_mutex_t               lock;
#define POOL_LOCK_INIT(pool)    pthread_mutex_init(&pool->lock, NULL)
#define POOL_LOCK_DEINIT(pool)  pthread_mutex_destroy(&pool->lock)
#define POOL_LOCK(pool)         pthread_mutex_lock(&pool->lock)
#define POOL_UNLOCK(pool)       pthread_mutex_unlock(&pool->lock)
#else
#define POOL_LOCK_INIT(pool)
#define POOL_LOCK_DEINIT(pool)
#define POOL_LOCK(pool)
#define POOL_UNLOCK(pool)
#endif
} mem_pool_t;

#ifdef MEM_POOL_NAIVE_PLUS
void mem_pool_cleanup(mem_pool_t *pool);
pool_addr_t mem_pool_alloc(mem_pool_t *pool, pool_size_t size);
void mem_pool_free(mem_pool_t *pool, pool_addr_t addr);
void mem_pool_create(mem_pool_t **pool, u64 total_size);
void mem_pool_destroy(mem_pool_t *pool);
pool_addr_t mem_pool_alloc_in_bank(mem_pool_t *pool, pool_size_t size);
#else
void mem_pool_cleanup(mem_pool_t *pool);
u64 mem_pool_alloc(mem_pool_t *pool, u64 size);
void mem_pool_free(mem_pool_t *pool, u64 addr);
void mem_pool_create(mem_pool_t **pool, u64 total_size);
void mem_pool_destroy(mem_pool_t *pool);
#endif

#endif /* BMDNN_MMPOOL_H_ */
