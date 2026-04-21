// // #include "types.h"
// // #include "param.h"
// // #include "memlayout.h"
// // #include "spinlock.h"
// // #include "riscv.h"
// // #include "defs.h"
// // #include "slab.h"

// // void print_kmem_cache(struct kmem_cache *cache, void (*slab_obj_printer)(void *))
// // {
// //   // TODO: Implement print_kmem_cache
// //   printf("[SLAB] TODO: print_kmem_cache is not yet implemented \n");
// // }

// // struct kmem_cache *kmem_cache_create(char *name, uint object_size)
// // {
// //   // TODO: Implement kmem_cache_create
// //   printf("[SLAB] TODO: kmem_cache_create is not yet implemented \n");
  
// //   struct kmem_cache *cache;
// //   cache = 0;
// //   return cache;
// // }

// // void kmem_cache_destroy(struct kmem_cache *cache)
// // {
// //   // TODO: Implement kmem_cache_destroy (will not be tested)
// //   printf("[SLAB] TODO: kmem_cache_destroy is not yet implemented \n");
// // }

// // void *kmem_cache_alloc(struct kmem_cache *cache)
// // {
// //   // TODO: Implement kmem_cache_alloc
// //   printf("[SLAB] TODO: kmem_cache_alloc is not yet implemented \n");
  
// //   // acquire(&cache->lock); // acquire the lock before modification
// //   // ... (modify kmem_cache)
// //   // release(&cache->lock); // release the lock before return
// //   return 0;
// // }

// // void kmem_cache_free(struct kmem_cache *cache, void *obj)
// // {
// //   // TODO: Implement kmem_cache_free
// //   printf("[SLAB] TODO: kmem_cache_free is not yet implemented \n");

// //   // acquire(&cache->lock); // acquire the lock before modification
// //   // ... (modify kmem_cache)
// //   // release(&cache->lock); // release the lock before return
// // }

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "slab.h"

#define SHUFFLE_MAX_OBJS 64

static struct spinlock slab_global_lock;
static int slab_global_lock_inited = 0;

static inline void
slab_lock_init_once(void)
{
  if(!slab_global_lock_inited){
    initlock(&slab_global_lock, "slab_global");
    slab_global_lock_inited = 1;
  }
}

static inline uint
align_up(uint x, uint a)
{
  return (x + a - 1) & ~(a - 1);
}

static inline uint64
align_up64(uint64 x, uint64 a)
{
  return (x + a - 1) & ~(a - 1);
}

static inline void
set_obj_next(void *obj, void *nxt)
{
  *(void **)obj = nxt;
}

static inline void *
get_obj_next(void *obj)
{
  return *(void **)obj;
}

static inline struct slab *
obj_to_slab(void *obj)
{
  return (struct slab *)PGROUNDDOWN((uint64)obj);
}

static inline uint
cache_slot_size(struct kmem_cache *cache)
{
  uint minsz = sizeof(void *);
  uint raw = cache->object_size < minsz ? minsz : cache->object_size;
  return align_up(raw, 8);
}

static inline uint
cache_max_objs(struct kmem_cache *cache)
{
  uint slot = cache_slot_size(cache);
  if(slot == 0 || PGSIZE <= sizeof(struct slab))
    return 0;
  return (PGSIZE - sizeof(struct slab)) / slot;
}

static inline uint
cache_in_cache_objs(struct kmem_cache *cache)
{
  uint slot = cache_slot_size(cache);
  uint64 off = align_up64((uint64)sizeof(struct kmem_cache), 8);
  if(slot == 0 || off >= PGSIZE)
    return 0;
  return (uint)((PGSIZE - off) / slot);
}

static inline void *
slab_obj_base(struct slab *s)
{
  return (void *)((char *)s + sizeof(struct slab));
}

static inline void *
slab_obj_at(struct slab *s, uint idx)
{
  return (void *)((char *)slab_obj_base(s) + (uint64)idx * (uint)s->slot_size);
}

static inline void *
cache_obj_base(struct kmem_cache *cache)
{
  uint64 off = align_up64((uint64)sizeof(struct kmem_cache), 8);
  return (void *)((char *)cache + off);
}

static inline void *
cache_obj_at(struct kmem_cache *cache, uint idx)
{
  return (void *)((char *)cache_obj_base(cache) + (uint64)idx * cache_slot_size(cache));
}

static inline uint
get_cache_freelist_off(struct kmem_cache *cache)
{
  return cache->meta32 & CACHE_FREELIST_MASK;
}

static inline void
set_cache_freelist_off(struct kmem_cache *cache, uint off)
{
  cache->meta32 = (cache->meta32 & ~CACHE_FREELIST_MASK) | (off & CACHE_FREELIST_MASK);
}

static inline uint
get_partial_cnt(struct kmem_cache *cache)
{
  return (cache->meta32 & CACHE_PARTIAL_MASK) >> CACHE_PARTIAL_SHIFT;
}

static inline void
set_partial_cnt(struct kmem_cache *cache, uint cnt)
{
  cache->meta32 =
      (cache->meta32 & ~CACHE_PARTIAL_MASK) |
      ((cnt << CACHE_PARTIAL_SHIFT) & CACHE_PARTIAL_MASK);
}

static inline void
inc_partial_cnt(struct kmem_cache *cache)
{
  set_partial_cnt(cache, get_partial_cnt(cache) + 1);
}

static inline void
dec_partial_cnt(struct kmem_cache *cache)
{
  uint cnt = get_partial_cnt(cache);
  if(cnt > 0)
    set_partial_cnt(cache, cnt - 1);
}

static inline void *
get_cache_freelist(struct kmem_cache *cache)
{
  uint off = get_cache_freelist_off(cache);
  if(off == 0)
    return 0;
  return (void *)((char *)cache + off);
}

static inline void
set_cache_freelist(struct kmem_cache *cache, void *obj)
{
  if(obj == 0){
    set_cache_freelist_off(cache, 0);
    return;
  }
  set_cache_freelist_off(cache, (uint)((char *)obj - (char *)cache));
}

static inline uint64
xorshift64(uint64 x)
{
  if(x == 0)
    x = 0x9e3779b97f4a7c15ULL;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return x;
}

static void
init_shuffled_freelist(void *base, uint n, uint stride, void **freelist_out)
{
  if(n == 0){
    *freelist_out = 0;
    return;
  }

  if(n > SHUFFLE_MAX_OBJS){
    for(uint i = 0; i < n; i++){
      void *cur = (void *)((char *)base + (uint64)i * stride);
      void *nxt = (i + 1 < n) ? (void *)((char *)base + (uint64)(i + 1) * stride) : 0;
      set_obj_next(cur, nxt);
    }
    *freelist_out = base;
    return;
  }

  void *arr[SHUFFLE_MAX_OBJS];

  for(uint i = 0; i < n; i++)
    arr[i] = (void *)((char *)base + (uint64)i * stride);

  uint64 seed = ((uint64)r_time() << 32) ^ (uint64)base ^
                ((uint64)stride << 16) ^ (uint64)n;
  if(seed == 0)
    seed = (uint64)base ^ 0xa0761d6478bd642fULL;

  for(uint i = n - 1; i > 0; i--){
    seed = xorshift64(seed);
    uint j = (uint)(seed % (i + 1));
    void *tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
  }

  for(uint i = 0; i < n; i++){
    void *cur = arr[i];
    void *nxt = (i + 1 < n) ? arr[i + 1] : 0;
    set_obj_next(cur, nxt);
  }

  *freelist_out = arr[0];
}

static struct slab *
slab_new(struct kmem_cache *cache)
{
  void *page = kalloc();
  if(page == 0)
    return 0;

  memset(page, 0, PGSIZE);

  struct slab *s = (struct slab *)page;
  s->freelist = 0;
  INIT_LIST_HEAD(&s->list);
  s->inuse = 0;
  s->max_objs = (ushort)cache_max_objs(cache);
  s->slot_size = (ushort)cache_slot_size(cache);
  s->reserved = 0;

  if(s->max_objs > 0)
    init_shuffled_freelist(slab_obj_base(s), s->max_objs, s->slot_size, &s->freelist);

  return s;
}

void
print_kmem_cache(struct kmem_cache *cache, void (*print_fn)(void *))
{
  if(cache == 0){
    printf("[SLAB] kmem_cache { name: (null), object_size: 0, at: 0x0000000000000000, in_cache_obj: 0 }\n");
    printf("[SLAB] print_kmem_cache end\n");
    return;
  }

  slab_lock_init_once();
  acquire(&slab_global_lock);

  uint in_cache_obj = cache_in_cache_objs(cache);

  printf("[SLAB] kmem_cache { name: %s, object_size: %u, at: %p, in_cache_obj: %u }\n",
         cache->name, cache->object_size, cache, in_cache_obj);

  if(in_cache_obj > 0){
    printf("[SLAB]    [ cache slabs ]\n");
    printf("[SLAB]        [ slab %p ] { freelist: %p, nxt: %p, max_objs: %u }\n",
           cache, get_cache_freelist(cache), (void *)0, in_cache_obj);

    if(print_fn != 0){
      for(uint i = 0; i < in_cache_obj; i++){
        void *addr = cache_obj_at(cache, i);
        uint64 as_ptr = *(uint64 *)addr;
        printf("[SLAB]           [ idx %u ] { addr: %p, as_ptr: %p, as_obj: {",
               i, addr, (void *)as_ptr);
        print_fn(addr);
        printf("} }\n");
      }
    }
  }

  if(!list_empty(&cache->partial)){
    printf("[SLAB]    [ partial slabs ]\n");

    struct slab *s;
    list_for_each_entry(s, &cache->partial, list){
      void *nxt = (void *)0;
      if(s->list.next != &cache->partial){
        struct slab *ns = list_entry(s->list.next, struct slab, list);
        nxt = (void *)ns;
      }

      printf("[SLAB]        [ slab %p ] { freelist: %p, nxt: %p, max_objs: %u }\n",
             s, s->freelist, nxt, (uint)s->max_objs);

      if(print_fn != 0){
        for(uint i = 0; i < (uint)s->max_objs; i++){
          void *addr = slab_obj_at(s, i);
          uint64 as_ptr = *(uint64 *)addr;
          printf("[SLAB]           [ idx %u ] { addr: %p, as_ptr: %p, as_obj: {",
                 i, addr, (void *)as_ptr);
          print_fn(addr);
          printf("} }\n");
        }
      }
    }
  }

  printf("[SLAB] print_kmem_cache end\n");
  release(&slab_global_lock);
}

struct kmem_cache *
kmem_cache_create(char *name, uint object_size)
{
  slab_lock_init_once();

  struct kmem_cache *cache = (struct kmem_cache *)kalloc();
  if(cache == 0)
    return 0;

  memset(cache, 0, PGSIZE);

  safestrcpy(cache->name, name, MP2_CACHE_MAX_NAME);
  cache->object_size = object_size;
  cache->meta32 = 0;
  INIT_LIST_HEAD(&cache->partial);

  uint in_cache_obj = cache_in_cache_objs(cache);
  if(in_cache_obj > 0){
    void *freelist = 0;
    init_shuffled_freelist(cache_obj_base(cache), in_cache_obj, cache_slot_size(cache), &freelist);
    set_cache_freelist(cache, freelist);
  }

  printf("[SLAB] New kmem_cache (name: %s, object size: %u bytes, at: %p, max objects per slab: %u, support in cache obj: %u) is created\n",
         cache->name, cache->object_size, cache, cache_max_objs(cache), in_cache_obj);

  return cache;
}

void *
kmem_cache_alloc(struct kmem_cache *cache)
{
  if(cache == 0)
    return 0;

  slab_lock_init_once();
  acquire(&slab_global_lock);

  printf("[SLAB] Alloc request on cache %s\n", cache->name);

  void *cache_obj = get_cache_freelist(cache);
  if(cache_obj != 0){
    set_cache_freelist(cache, get_obj_next(cache_obj));
    memset(cache_obj, 0, cache->object_size);

    printf("[SLAB] Object %p in slab %p (%s) is allocated and initialized\n",
           cache_obj, cache, cache->name);

    release(&slab_global_lock);
    return cache_obj;
  }

  for(;;){
    struct slab *s;

    if(list_empty(&cache->partial)){
      s = slab_new(cache);
      if(s == 0){
        release(&slab_global_lock);
        return 0;
      }

      printf("[SLAB] A new slab %p (%s) is allocated\n", s, cache->name);

      list_add_tail(&s->list, &cache->partial);
      inc_partial_cnt(cache);
    }

    s = list_first_entry(&cache->partial, struct slab, list);

    if(s->freelist == 0){
      list_del_init(&s->list);
      dec_partial_cnt(cache);
      continue;
    }

    void *obj = s->freelist;
    s->freelist = get_obj_next(obj);
    s->inuse++;

    if(s->inuse == s->max_objs){
      list_del_init(&s->list);
      dec_partial_cnt(cache);
    }

    memset(obj, 0, cache->object_size);

    printf("[SLAB] Object %p in slab %p (%s) is allocated and initialized\n",
           obj, s, cache->name);

    release(&slab_global_lock);
    return obj;
  }
}

void
kmem_cache_free(struct kmem_cache *cache, void *obj)
{
  if(cache == 0 || obj == 0)
    return;

  slab_lock_init_once();
  acquire(&slab_global_lock);

  if(PGROUNDDOWN((uint64)obj) == PGROUNDDOWN((uint64)cache)){
    printf("[SLAB] Free %p in slab %p (%s)\n", obj, cache, cache->name);

    set_obj_next(obj, get_cache_freelist(cache));
    set_cache_freelist(cache, obj);

    printf("[SLAB] End of free\n");
    release(&slab_global_lock);
    return;
  }

  struct slab *s = obj_to_slab(obj);
  struct slab *to_free = 0;

  printf("[SLAB] Free %p in slab %p (%s)\n", obj, s, cache->name);

  if(s->inuse == s->max_objs){
    list_add(&s->list, &cache->partial);
    inc_partial_cnt(cache);
  }

  set_obj_next(obj, s->freelist);
  s->freelist = obj;

  if(s->inuse > 0)
    s->inuse--;

  if(s->inuse == 0 && get_partial_cnt(cache) > MP2_MIN_AVAIL_SLAB){
    list_del_init(&s->list);
    dec_partial_cnt(cache);
    to_free = s;
    printf("[SLAB] Slab %p (%s) is freed due to save memory\n", s, cache->name);
  }

  printf("[SLAB] End of free\n");
  release(&slab_global_lock);

  if(to_free != 0)
    kfree((void *)to_free);
}

void
kmem_cache_destroy(struct kmem_cache *cache)
{
  if(cache == 0)
    return;

  slab_lock_init_once();
  acquire(&slab_global_lock);

  if(!list_empty(&cache->partial)){
    struct slab *s, *tmp;
    list_for_each_entry_safe(s, tmp, &cache->partial, list){
      list_del_init(&s->list);
      kfree((void *)s);
    }
  }

  release(&slab_global_lock);
  kfree((void *)cache);
}