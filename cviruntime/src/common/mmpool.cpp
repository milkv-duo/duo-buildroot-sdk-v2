#include "mmpool.h"

//#define MEM_POOL_DEBUG

#ifdef MEM_POOL_NAIVE
void mem_pool_cleanup(mem_pool_t *pool)
{
  pool->head_addr = 0;
  pool->head_slot = 0;
  pool->slot_used = 0;
  for ( int i = 0; i < MEM_POOL_SLOT_NUM; i++ ) {
    pool->slot[i] = MEM_POOL_ADDR_INVALID;
  }
}

u64 mem_pool_alloc(mem_pool_t *pool, u64 size)
{
  //assert(size % GLOBAL_MEM_ALIGN_SIZE == 0); // FIXME(wwcai): support 1byte align
  POOL_LOCK(pool);
  /* single direction, no looking up for empty slot */
  int cur_slot = pool->head_slot;
  if (cur_slot >= MEM_POOL_SLOT_NUM) {
    assert(0);  // no error handling yet
    return MEM_POOL_ADDR_INVALID;
  }

  u64 cur_addr = pool->head_addr;
  if (cur_addr + size > pool->total_size) {
    assert(0);  // no error handling yet
    return MEM_POOL_ADDR_INVALID;
  }

  pool->slot[cur_slot] = cur_addr;
  pool->head_addr += size;
  pool->head_slot++;
  pool->slot_used++;

  POOL_UNLOCK(pool);
#ifdef MEM_POOL_DEBUG
  printf("mem_pool: alloc addr 0x%16x on slot %d\n", cur_addr, cur_slot);
#endif
  return cur_addr;
}

void mem_pool_free(mem_pool_t *pool, u64 addr)
{
  POOL_LOCK(pool);
  /* lookup for addr for sanity checking */
  int cur_slot = MEM_POOL_SLOT_NUM;
  for ( int i = 0; i < pool->head_slot; i++ ) {
    if (pool->slot[i] == addr) {
      cur_slot = i;
      break;
    }
  }
  if (cur_slot == MEM_POOL_SLOT_NUM) {
    printf("mem_pool: No matching slot for free, addr = 0x%16llx\n", addr);
    assert(0);
  }
  pool->slot[cur_slot] = MEM_POOL_ADDR_INVALID;

#ifdef MEM_POOL_DEBUG
  printf("mem_pool: free addr 0x%16x on slot %d\n", addr, cur_slot);
#endif

  assert(pool->slot_used);
  pool->slot_used--;
  if ( pool->slot_used == 0 ) {
    mem_pool_cleanup(pool);
#ifdef MEM_POOL_DEBUG
    printf("mem_pool: cleanup\n");
#endif
  }
  POOL_UNLOCK(pool);
}

void mem_pool_create(mem_pool_t **pool, u64 total_size)
{
  mem_pool_t *tpool = new mem_pool_t;
  POOL_LOCK_INIT(tpool);

  tpool->total_size = total_size;
  mem_pool_cleanup(tpool);

  *pool = tpool;
}

void mem_pool_destroy(mem_pool_t *pool)
{
  /* sanity checking */
  assert(pool->slot_used == 0);
  POOL_LOCK_DEINIT(pool);

  delete pool;
}
#endif /* MEM_POOL_NAIVE */

#ifdef MEM_POOL_ZEPHRE

/* Auto-Defrag settings */

#define AD_NONE 0
#define AD_BEFORE_SEARCH4BIGGERBLOCK 1
#define AD_AFTER_SEARCH4BIGGERBLOCK 2

#define AUTODEFRAG AD_AFTER_SEARCH4BIGGERBLOCK

#define OCTET_TO_SIZEOFUNIT(X) (X)

#define MAX_BLOCK_SIZE      (1024 * 1024 * 1024)
#define MIN_BLOCK_SIZE      (4 * 1024)

static void _k_mem_pool_init_pre(struct pool_struct *P, u64 total_size)
{
    assert(total_size % MAX_BLOCK_SIZE == 0);

    P->maxblock_size = MAX_BLOCK_SIZE;
    P->minblock_size = MIN_BLOCK_SIZE;
    P->nr_of_maxblocks = total_size/P->maxblock_size;
    P->nr_of_block_sets = 0;
    P->bufblock = (pool_addr_t)0;

    /* deternmine nr_of_block_sets */
    int nr_of_block_sets = 1;
    int num = MAX_BLOCK_SIZE/MIN_BLOCK_SIZE;
    while(num >>= 2) {
        nr_of_block_sets++;
    }
    P->block_set = malloc(nr_of_block_sets * sizeof(struct pool_block_set));
    assert(P->block_set);

#ifdef MEM_POOL_DEBUG
    printf("mem_pool: total_size %lld, max %d, min %d, nr_max %d\n",
        total_size, P->maxblock_size, P->minblock_size, P->nr_of_maxblocks);
#endif

    int block_size = MAX_BLOCK_SIZE;
    int nr_of_entries = (P->nr_of_maxblocks + 3) / 4;
    int i = 0;

    while (block_size >= P->minblock_size) {
#ifdef MEM_POOL_DEBUG
        printf("mem_pool: alloc block_set for size %d, nr %d\n",
            block_size, nr_of_entries);
#endif
        P->block_set[i].block_size = block_size;
        P->block_set[i].nr_of_entries = nr_of_entries;
        P->block_set[i].quad_block = malloc(nr_of_entries * sizeof(struct pool_quad_block));
        assert(P->block_set[i].quad_block);
        for (int t = 0; t < nr_of_entries; t++) {
            P->block_set[i].quad_block[t].mem_blocks = MEM_POOL_ADDR_INVALID;
            P->block_set[i].quad_block[t].mem_status = 0;
        }
#ifdef CONFIG_OBJECT_MONITOR
        P->block_set[i].count = 0;
#endif
        i++;
        P->nr_of_block_sets++;
        block_size = block_size >> 2;
        nr_of_entries = nr_of_entries << 2;
    }

#ifdef MEM_POOL_DEBUG
    printf("mem_pool: init %d block_set\n", nr_of_block_sets);
#endif
    assert(nr_of_block_sets == P->nr_of_block_sets);
}

static void _k_mem_pool_exit_post(struct pool_struct *P)
{
    for (int i = 0; i < P->nr_of_block_sets; i++) {
        free(P->block_set[i].quad_block);
    }
    free(P->block_set);
}

static void _k_mem_pool_init(mem_pool_t *pool)
{
    struct pool_struct *P;
    int i;

    /* perform initialization for each memory pool */

    for (i = 0, P = pool->_k_mem_pool_list; i < pool->_k_mem_pool_count; i++, P++) {

        /*
         * mark block set for largest block size
         * as owning all of the memory pool buffer space
         */

        int remaining = P->nr_of_maxblocks;
        int t = 0;
        pool_addr_t memptr = P->bufblock;

#ifdef MEM_POOL_DEBUG
        printf("mem_pool: remaining %d\n", remaining);
#endif

        while (remaining >= 4) {
            P->block_set[0].quad_block[t].mem_blocks = memptr;
            P->block_set[0].quad_block[t].mem_status = 0xF;
            t++;
            remaining = remaining - 4;
            memptr +=
                OCTET_TO_SIZEOFUNIT(P->block_set[0].block_size)
                * 4;
        }

        if (remaining != 0) {
            P->block_set[0].quad_block[t].mem_blocks = memptr;
            P->block_set[0].quad_block[t].mem_status =
                0xF >> (4 - remaining);
            /* non-existent blocks are marked as unavailable */
        }

        /*
         * note: all other block sets own no blocks, since their
         * first quad-block has a NULL memory pointer
         */
#ifdef MEM_POOL_DEBUG
        printf("mem_pool: remaining %d, t = %d\n", remaining, t);
#endif
    }
}

static u64 compute_block_set_index(struct pool_struct *P, u32 data_size)
{
    u32 block_size = P->minblock_size;
    u32 offset = P->nr_of_block_sets - 1;

    while (data_size > block_size) {
        block_size = block_size << 2;
        offset--;
    }

    return offset;
}

static void free_existing_block(pool_addr_t ptr, struct pool_struct *P, int index)
{
    struct pool_quad_block *quad_block = P->block_set[index].quad_block;
    pool_addr_t block_ptr;
    int i, j;

    /*
     * search block set's quad-blocks until the block is located,
     * then mark it as unused
     *
     * note: block *must* exist, so no need to do array bounds checking
     */

    for (i = 0; ; i++) {
        assert((i < P->block_set[index].nr_of_entries) &&
             (quad_block[i].mem_blocks != MEM_POOL_ADDR_INVALID));

        block_ptr = quad_block[i].mem_blocks;
        for (j = 0; j < 4; j++) {
            if (ptr == block_ptr) {
                quad_block[i].mem_status |= (1 << j);
                return;
            }
            block_ptr += OCTET_TO_SIZEOFUNIT(
                P->block_set[index].block_size);
        }
    }
}

static void defrag(struct pool_struct *P,
           int ifraglevel_start, int ifraglevel_stop)
{
    struct pool_quad_block *quad_block;

    /* process block sets from smallest to largest permitted sizes */

    for (int j = ifraglevel_start; j > ifraglevel_stop; j--) {

        quad_block = P->block_set[j].quad_block;
        int i = 0;

        do {
            /* block set is done if no more quad-blocks exist */

            if (quad_block[i].mem_blocks == MEM_POOL_ADDR_INVALID) {
                break;
            }

            /* reassemble current quad-block, if possible */

            if (quad_block[i].mem_status == 0xF) {

                /*
                 * mark the corresponding block in next larger
                 * block set as free
                 */

                free_existing_block(
                    quad_block[i].mem_blocks, P, j - 1);

                /*
                 * delete the quad-block from this block set
                 * by replacing it with the last quad-block
                 *
                 * (algorithm works even when the deleted
                 * quad-block is the last quad_block)
                 */

                int k = i;
                while (((k+1) != P->block_set[j].nr_of_entries)
                       &&
                       (quad_block[k+1].mem_blocks != MEM_POOL_ADDR_INVALID)) {
                    k++;
                }

                quad_block[i].mem_blocks =
                    quad_block[k].mem_blocks;
                quad_block[i].mem_status =
                    quad_block[k].mem_status;

                quad_block[k].mem_blocks = MEM_POOL_ADDR_INVALID;

                /* loop & process replacement quad_block[i] */
            } else {
                i++;
            }

            /* block set is done if at end of quad-block array */

        } while (i < P->block_set[j].nr_of_entries);
    }
}

#ifdef __arm__
static inline unsigned int find_lsb_set(u32 op)
{
        unsigned int bit;

        __asm__ volatile(
                "rsb %0, %1, #0;\n\t"
                "ands %0, %0, %1;\n\t" /* r0 = x & (-x): only LSB set */
                "itt ne;\n\t"
                "   clzne %0, %0;\n\t" /* count leading zeroes */
                "   rsbne %0, %0, #32;\n\t"
                : "=&r"(bit)
                : "r"(op));

        return bit;
}
#endif /* __arm__ */
#ifdef __x86_64__
static inline unsigned int find_lsb_set(u32 op)
{
        unsigned int bitpos;

        __asm__ volatile (

#if defined(CONFIG_CMOV)

                "bsfl %1, %0;\n\t"
                "cmovzl %2, %0;\n\t"
                : "=r" (bitpos)
                : "rm" (op), "r" (-1)
                : "cc"

#else

                  "bsfl %1, %0;\n\t"
                  "jnz 1f;\n\t"
                  "movl $-1, %0;\n\t"
                  "1:\n\t"
                : "=r" (bitpos)
                : "rm" (op)
                : "cc"

#endif /* CONFIG_CMOV */
                );

        return (bitpos + 1);
}
#endif /* __x86_64__ */

static pool_addr_t get_existing_block(struct pool_block_set *pfraglevelinfo,
                int *piblockindex)
{
    pool_addr_t found = MEM_POOL_ADDR_INVALID;
    int i = 0;
    int free_bit;

    do {
        /* give up if no more quad-blocks exist */

        if (pfraglevelinfo->quad_block[i].mem_blocks == MEM_POOL_ADDR_INVALID) {
            break;
        }

        /* allocate a block from current quad-block, if possible */

        int status = pfraglevelinfo->quad_block[i].mem_status;
        if (status != 0x0) {
            /* identify first free block */
            free_bit = find_lsb_set(status) - 1;

            /* compute address of free block */
            found = (u64)pfraglevelinfo->quad_block[i].mem_blocks +
                (u64)(OCTET_TO_SIZEOFUNIT(free_bit * (u64)pfraglevelinfo->block_size));

            /* mark block as unavailable (using XOR to invert) */
            pfraglevelinfo->quad_block[i].mem_status ^=
                1 << free_bit;
#ifdef CONFIG_OBJECT_MONITOR
            pfraglevelinfo->count++;
#endif
            break;
        }

        /* move on to next quad-block; give up if at end of array */

    } while (++i < pfraglevelinfo->nr_of_entries);

    *piblockindex = i;
    return found;
}

static pool_addr_t get_block_recursive(struct pool_struct *P,
                 int index, int startindex)
{
    int i;
    pool_addr_t found, larger_block;
    struct pool_block_set *fr_table;

    /* give up if we've exhausted the set of maximum size blocks */

    if (index < 0) {
        return MEM_POOL_ADDR_INVALID;
    }

    /* try allocating a block from the current block set */

    fr_table = P->block_set;
    i = 0;

    found = get_existing_block(&(fr_table[index]), &i);
    if (found != MEM_POOL_ADDR_INVALID) {
        return found;
    }

#if AUTODEFRAG == AD_BEFORE_SEARCH4BIGGERBLOCK
    /*
     * do a partial defragmentation of memory pool & try allocating again
     * - do this on initial invocation only, not recursive ones
     *   (since there is no benefit in repeating the defrag)
     * - defrag only the blocks smaller than the desired size,
     *   and only until the size needed is reached
     *
     * note: defragging at this time tries to preserve the memory pool's
     * larger blocks by fragmenting them only when necessary
     * (i.e. at the cost of doing more frequent auto-defragmentations)
     */

    if (index == startindex) {
        defrag(P, P->nr_of_block_sets - 1, startindex);
        found = get_existing_block(&(fr_table[index]), &i);
        if (found != MEM_POOL_ADDR_INVALID) {
            return found;
        }
    }
#endif

    /* try allocating a block from the next largest block set */

    larger_block = get_block_recursive(P, index - 1, startindex);
    if (larger_block != MEM_POOL_ADDR_INVALID) {
        /*
         * add a new quad-block to the current block set,
         * then mark one of its 4 blocks as used and return it
         *
         * note: "i" was earlier set to indicate the first unused
         * quad-block entry in the current block set
         */

        fr_table[index].quad_block[i].mem_blocks = larger_block;
        fr_table[index].quad_block[i].mem_status = 0xE;
#ifdef CONFIG_OBJECT_MONITOR
        fr_table[index].count++;
#endif
        return larger_block;
    }

#if AUTODEFRAG == AD_AFTER_SEARCH4BIGGERBLOCK
    /*
     * do a partial defragmentation of memory pool & try allocating again
     * - do this on initial invocation only, not recursive ones
     *   (since there is no benefit in repeating the defrag)
     * - defrag only the blocks smaller than the desired size,
     *   and only until the size needed is reached
     *
     * note: defragging at this time tries to limit the cost of doing
     * auto-defragmentations by doing them only when necessary
     * (i.e. at the cost of fragmenting the memory pool's larger blocks)
     */

    if (index == startindex) {
        defrag(P, P->nr_of_block_sets - 1, startindex);
        found = get_existing_block(&(fr_table[index]), &i);
        if (found != MEM_POOL_ADDR_INVALID) {
            return found;
        }
    }
#endif

    return MEM_POOL_ADDR_INVALID; /* can't find (or create) desired block */
}

/* FIXME: need do sort for performance */
static int find_slot(mem_pool_t *pool, u64 addr)
{
    for (int i = 0; i < MEM_POOL_SLOT_NUM; i++) {
        if (pool->slot_addr[i] == addr)
            return i;
    }
    /* failed to find */
    assert(0);
}

static int find_empty_slot(mem_pool_t *pool)
{
    for (int i = 0; i < MEM_POOL_SLOT_NUM; i++) {
        if (pool->slot_size[i] == 0) {
            assert(pool->slot_addr[i] == MEM_POOL_ADDR_INVALID);
            return i;
        }
    }
    /* failed to find empty slot */
    assert(0);
}

static void take_slot(mem_pool_t *pool, int i, u64 addr, u32 size)
{
    assert(i >= 0 && i < MEM_POOL_SLOT_NUM);
    pool->slot_used++;
    pool->slot_addr[i] = addr;
    pool->slot_size[i] = size;
    assert(pool->slot_used <= MEM_POOL_SLOT_NUM);
}

static void give_slot(mem_pool_t *pool, int i)
{
    assert(i >= 0 && i < MEM_POOL_SLOT_NUM);
    assert(pool->slot_used > 0);
    pool->slot_addr[i] = MEM_POOL_ADDR_INVALID;
    pool->slot_size[i] = 0;
    pool->slot_used--;
}

u64 mem_pool_alloc(mem_pool_t *pool, u64 size)
{
    //assert(size % GLOBAL_MEM_ALIGN_SIZE == 0); // FIXME(wwcai): support 1byte align
    POOL_LOCK(pool);

    struct pool_struct *P = &pool->_k_mem_pool_list[0];
    pool_addr_t found_block;
    int offset;

#ifdef MEM_POOL_DEBUG
    printf("mem_pool: alloc req_size %lld\n", size);
#endif
    assert(size <= P->maxblock_size);
    /* locate block set to try allocating from */

    offset = compute_block_set_index(P, size);

    /* allocate block (fragmenting a larger block, if needed) */

    found_block = get_block_recursive(P, offset, offset);

    if (found_block == MEM_POOL_ADDR_INVALID) {
        return MEM_POOL_ADDR_INVALID;
    }

#ifdef MEM_POOL_DEBUG
    printf("mem_pool: alloc addr 0x%llx size %lld\n", found_block, size);
#endif

    int slot = find_empty_slot(pool);
    take_slot(pool, slot, (u64)found_block, size);

    POOL_UNLOCK(pool);
    return (u64)found_block;
}

void mem_pool_free(mem_pool_t *pool, u64 addr)
{
    POOL_LOCK(pool);

    struct pool_struct *P = &pool->_k_mem_pool_list[0];
    u64 offset;

    int slot = find_slot(pool, addr);
    int size = pool->slot_size[slot];

#ifdef MEM_POOL_DEBUG
    printf("mem_pool: free addr 0x%llx, found size %d\n", addr, size);
#endif
    /* determine block set that block belongs to */

    offset = compute_block_set_index(P, size);

    /* mark the block as unused */

    free_existing_block(addr, P, offset);

    give_slot(pool, slot);

    POOL_UNLOCK(pool);
}

void mem_pool_create(mem_pool_t **pool, u64 total_size)
{
  mem_pool_t *tpool = new mem_pool_t;
  POOL_LOCK_INIT(tpool);
  tpool->total_size = total_size;

  struct pool_struct *P = &tpool->_k_mem_pool_list[0];
  _k_mem_pool_init_pre(P, total_size);
  tpool->_k_mem_pool_count = 1;
  _k_mem_pool_init(tpool);

  tpool->slot_used = 0;
  for (int i = 0; i < MEM_POOL_SLOT_NUM; i++) {
    tpool->slot_addr[i] = MEM_POOL_ADDR_INVALID;
    tpool->slot_size[i] = 0;
  }

  *pool = tpool;

#ifdef MEM_POOL_DEBUG
  printf("mem_pool: create\n");
#endif
}

void mem_pool_destroy(mem_pool_t *pool)
{
#ifdef MEM_POOL_DEBUG
  printf("mem_pool: destroy\n");
#endif
  POOL_LOCK(pool);

  /* sanity checking */
  if (pool->slot_used) {
    printf("mem_pool: destroy pool with %d left\n", pool->slot_used);
    for (int i = 0; i < MEM_POOL_SLOT_NUM; i++) {
      if (pool->slot_size[i] != 0) {
        printf("mem_pool:   slot %d in use, size %d\n", i, pool->slot_size[i]);
      }
    }
  }
  assert(pool->slot_used == 0);

  struct pool_struct *P = &pool->_k_mem_pool_list[0];
  k_mem_pool_exit_post(P);

  POOL_UNLOCK(pool);
  POOL_LOCK_DEINIT(pool);

  delete pool;
}
#endif /* MEM_POOL_ZEPHRE */

#ifdef MEM_POOL_NAIVE_PLUS
void mem_pool_init(mem_pool_t *pool)
{
  struct pool_struct *P = &pool->_mem_pool_list[0];
  P->slot_avail.clear();
  P->slot_in_use.clear();
  P->slot_avail.push_back(make_pair(0, pool->total_size));
}

/* look for the smallest yet sufficient memory slot for the demanding size */
static pool_addr_t find_slot(struct pool_struct *P, pool_size_t size){
  assert(size % MIN_SLOT_SIZE == 0);

  vector<pool_pair_t>::iterator it, it_min = P->slot_avail.end();
  pool_size_t min_size_among_sufficient = 0;
  for(it = P->slot_avail.begin(); it != P->slot_avail.end(); ++it){
    if((*it).second >= size){
      if(min_size_among_sufficient == 0 || (*it).second < min_size_among_sufficient){
        it_min = it;
        min_size_among_sufficient = (*it).second;
      }
    }
  }

  if(it_min == P->slot_avail.end() || (*it_min).second == 0){
    printf("Memory exhausted: cannot find a slot.\n");
    return MEM_POOL_ADDR_INVALID;
  }
  pool_addr_t addr = (*it_min).first;

  if((*it_min).second == size){
    P->slot_avail.erase(it_min);
  } else {
    (*it_min).first = addr + size;
    (*it_min).second -= size;
  }

  P->slot_in_use.insert(make_pair(addr, size));

  return addr;
}

pool_addr_t mem_pool_alloc(mem_pool_t *pool, pool_size_t size)
{
  //assert(size % GLOBAL_MEM_ALIGN_SIZE == 0); // FIXME(wwcai): support 1byte align
  POOL_LOCK(pool);

  struct pool_struct *P = &pool->_mem_pool_list[0];
  assert(P->num_slots_in_use < MEM_POOL_SLOT_NUM);
  pool_size_t size_to_alloc = (size + MIN_SLOT_SIZE -1) / MIN_SLOT_SIZE * MIN_SLOT_SIZE;
  pool_addr_t addr_to_alloc = find_slot(P, size_to_alloc);

  if (addr_to_alloc == MEM_POOL_ADDR_INVALID) {
#ifdef MEM_POOL_DEBUG
    printf("mem_pool: mem alloc failed in searching stage\n");
#endif
    assert(0);  // no error handling yet
    return MEM_POOL_ADDR_INVALID;
  }
  else if (addr_to_alloc + size_to_alloc > pool->total_size) {
#ifdef MEM_POOL_DEBUG
    printf("mem_pool: mem alloc insufficient size\n");
#endif
    assert(0);  // no error handling yet
    return MEM_POOL_ADDR_INVALID;
  }

  P->num_slots_in_use++;
#ifdef MEM_POOL_DEBUG
  printf("mem_pool: alloc addr 0x%lx with size of %ld; actual size required = %ld\n",
      addr_to_alloc, size_to_alloc, size);
#endif

  POOL_UNLOCK(pool);
  return addr_to_alloc;
}

void mem_pool_free(mem_pool_t *pool, pool_addr_t addr_to_free)
{
  POOL_LOCK(pool);

  struct pool_struct *P = &pool->_mem_pool_list[0];
  pool_map_t::iterator it = P->slot_in_use.find(addr_to_free);
  assert(it != P->slot_in_use.end());
  pool_size_t size_to_free = P->slot_in_use[addr_to_free];
  assert(size_to_free % MIN_SLOT_SIZE == 0);
  P->slot_in_use.erase(it);
  P->num_slots_in_use--;

  pool_addr_t addr_next = addr_to_free + size_to_free;
  vector<pool_pair_t>::iterator it_prev_slot = find_if(P->slot_avail.begin(),
      P->slot_avail.end(), offset_prev_finder(addr_to_free));
  vector<pool_pair_t>::iterator it_next_slot = find_if(P->slot_avail.begin(),
      P->slot_avail.end(), offset_next_finder(addr_next));

  if(it_prev_slot == P->slot_avail.end() && it_next_slot == P->slot_avail.end()){
    P->slot_avail.push_back(make_pair(addr_to_free, size_to_free));
  }
  else if(it_prev_slot == P->slot_avail.end()){
    (*it_next_slot).first = addr_to_free;
    (*it_next_slot).second += size_to_free;
#ifdef DEBUG
    CHECK_FREED_MEM((*it_next_slot).first, (*it_next_slot).second, P);
#endif
  }
  else if(it_next_slot == P->slot_avail.end()){
    (*it_prev_slot).second += size_to_free;
#ifdef DEBUG
    CHECK_FREED_MEM((*it_prev_slot).first, (*it_prev_slot).second, P);
#endif
  } else {
    (*it_prev_slot).second += (size_to_free + (*it_next_slot).second);
    P->slot_avail.erase(it_next_slot);
  }

#ifdef MEM_POOL_DEBUG
  printf("mem_pool_free: addr_to_free = 0x%lx; size_to_free = %ld\n",
      addr_to_free, size_to_free);
#endif

  POOL_UNLOCK(pool);
}

void mem_pool_create(mem_pool_t **pool, u64 total_size)
{
  mem_pool_t *tpool = new mem_pool_t;
  POOL_LOCK_INIT(tpool);

  tpool->total_size = total_size;
  mem_pool_init(tpool);
  tpool->_mem_pool_count = 1;
  struct pool_struct *P = &tpool->_mem_pool_list[0];
  P->num_slots_in_use = 0;

  *pool = tpool;

#ifdef MEM_POOL_DEBUG
  printf("mem_pool: create\n");
#endif
}

void mem_pool_destroy(mem_pool_t *pool)
{
  POOL_LOCK(pool);

  /* sanity checking */
  struct pool_struct *P = &pool->_mem_pool_list[0];
  assert(P->slot_avail[0].first == 0);
  assert(P->slot_avail[0].second == pool->total_size);
  assert(P->slot_in_use.empty());
  assert(P->num_slots_in_use == 0);

  POOL_UNLOCK(pool);
  POOL_LOCK_DEINIT(pool);

  delete pool;
}

static bool slot_in_bank(pool_addr_t addr, pool_size_t size)
{
  pool_addr_t aligned_addr = addr % BANK_SIZE;
  return ((aligned_addr + size) <= BANK_SIZE);
}

static pool_addr_t find_slot_in_bank(struct pool_struct *P, pool_size_t size_to_alloc)
{
  assert(size_to_alloc % MIN_SLOT_SIZE == 0);

  vector<pool_pair_t>::iterator it, it_find = P->slot_avail.end();
  int find_case = 0;
  for (it = P->slot_avail.begin(); it != P->slot_avail.end(); ++it) {
      if ((*it).second >= size_to_alloc) {
        pool_addr_t slot_addr = (*it).first;
        pool_size_t slot_size = (*it).second;
        if (slot_in_bank(slot_addr, size_to_alloc)) {
            find_case = 1;
        } else if(slot_in_bank(slot_addr + slot_size - size_to_alloc, size_to_alloc)) {
            find_case = 2;
        }
        if (find_case > 0) {
            it_find = it;
            break;
        }
      }
  }

  if (find_case == 0) {
    printf("Memory exhausted: cannot find a slot.\n");
    return MEM_POOL_ADDR_INVALID;
  }

  pool_addr_t addr;
  if (find_case == 1) {
    addr = (*it_find).first;
  } else {
    addr = (*it_find).first + (*it_find).second - size_to_alloc;
  }

  if ((*it_find).second == size_to_alloc) {
    P->slot_avail.erase(it_find);
  } else {
    if (find_case == 1) {
        (*it_find).first += size_to_alloc;
        (*it_find).second -= size_to_alloc;
    } else {
        (*it_find).second -= size_to_alloc;
    }
  }

  P->slot_in_use.insert(make_pair(addr, size_to_alloc));

  return addr;
}

pool_addr_t mem_pool_alloc_in_bank(mem_pool_t *pool, pool_size_t size)
{
  //assert(size % GLOBAL_MEM_ALIGN_SIZE == 0); // FIXME(wwcai): support 1byte align
  POOL_LOCK(pool);

  struct pool_struct *P = &pool->_mem_pool_list[0];
  assert(P->num_slots_in_use < MEM_POOL_SLOT_NUM);
  pool_size_t size_to_alloc = (size + MIN_SLOT_SIZE -1) / MIN_SLOT_SIZE * MIN_SLOT_SIZE;
  pool_addr_t addr_to_alloc = find_slot_in_bank(P, size_to_alloc);

  if (addr_to_alloc == MEM_POOL_ADDR_INVALID) {
#ifdef MEM_POOL_DEBUG
    printf("mem_pool: mem alloc failed in searching stage\n");
#endif
    assert(0);  // no error handling yet
    return MEM_POOL_ADDR_INVALID;
  }
  else if (addr_to_alloc + size_to_alloc > pool->total_size) {
#ifdef MEM_POOL_DEBUG
    printf("mem_pool: mem alloc insufficient size\n");
#endif
    assert(0);  // no error handling yet
    return MEM_POOL_ADDR_INVALID;
  }

  P->num_slots_in_use++;
#ifdef MEM_POOL_DEBUG
  printf("mem_pool: alloc addr 0x%lx with size of %ld; actual size required = %ld\n",
      addr_to_alloc, size_to_alloc, size);
#endif

  POOL_UNLOCK(pool);
  return addr_to_alloc;
}

#endif /* MEM_POOL_NAIVE_PLUS */
