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


// #include "types.h"
// #include "param.h"
// #include "memlayout.h"
// #include "spinlock.h"
// #include "riscv.h"
// #include "defs.h"
// #include "slab.h"

// // --- Internal helpers (basic part implementation; no bonus features) ---

// static inline uint
// align_up(uint x, uint a)
// {
//   return (x + a - 1) & ~(a - 1);
// }

// // The freelist stores the next pointer in the first machine word.
// static inline void
// set_obj_next(void *obj, void *nxt)
// {
//   *(void **)obj = nxt;
// }

// static inline void *
// get_obj_next(void *obj)
// {
//   return *(void **)obj;
// }

// // Map an object address back to its containing slab page in O(1).
// // (Pages returned by kalloc() are page-aligned.)
// static inline struct slab *
// obj_to_slab(void *obj)
// {
//   return (struct slab *)PGROUNDDOWN((uint64)obj);
// }

// static inline void *
// slab_obj_base(struct slab *s)
// {
//   return (void *)((char *)s + sizeof(struct slab));
// }

// static inline void *
// slab_obj_at(struct slab *s, uint idx)
// {
//   return (void *)((char *)slab_obj_base(s) + (uint64)idx * s->slot_size);
// }

// static int
// list_remove_slab(struct slab **head, struct slab *target)
// {
//   struct slab **pp = head;
//   while(*pp && *pp != target)
//     pp = &(*pp)->nxt;
//   if(*pp == 0)
//     return 0;
//   *pp = target->nxt;
//   target->nxt = 0;
//   return 1;
// }

// static struct slab *
// slab_new(struct kmem_cache *cache)
// {
//   void *page = kalloc();
//   if(page == 0)
//     return 0;

//   struct slab *s = (struct slab *)page;

//   // Initialize slab header.
//   s->nxt = 0;
//   s->inuse = 0;
//   s->max_objs = cache->max_objs;
//   s->slot_size = cache->slot_size;
//   s->reserved = 0;

//   // Build freelist (linear order is fine for the basic part).
//   if(s->max_objs == 0){
//     s->freelist = 0;
//     return s;
//   }

//   for(uint i = 0; i < s->max_objs; i++){
//     void *cur = slab_obj_at(s, i);
//     void *nxt = (i + 1 < s->max_objs) ? slab_obj_at(s, i + 1) : 0;
//     set_obj_next(cur, nxt);
//   }
//   s->freelist = slab_obj_at(s, 0);
//   return s;
// }

// // void print_kmem_cache(struct kmem_cache *cache, void (*slab_obj_printer)(void *))
// // {
// //   if(cache == 0){
// //     printf("[SLAB] kmem_cache { name: (null), object_size: 0, at: 0x0000000000000000, in_cache_obj: 0 }\n");
// //     printf("[SLAB] print_kmem_cache end\n");
// //     return;
// //   }

// //   // Basic info
// //   printf("[SLAB] kmem_cache { name: %s, object_size: %d, at: %p, in_cache_obj: %d }\n",
// //          cache->name, cache->object_size, cache, cache->in_cache_obj);

// //   // For the basic part, we only output partial slabs.
// //   printf("[SLAB]    [ partial slabs ]\n");

// //   // Traverse partial slabs in link order.
// //   for(struct slab *s = cache->partial; s != 0; s = s->nxt){
// //     printf("[SLAB]        [ slab %p ] { freelist: %p, nxt: %p, max_objs: %d }\n",
// //            s, s->freelist, s->nxt, s->max_objs);

// //     if(slab_obj_printer == 0)
// //       continue;

// //     // Print all object blocks in address order low -> high.
// //     for(uint i = 0; i < s->max_objs; i++){
// //       void *addr = slab_obj_at(s, i);
// //       uint64 as_ptr = *(uint64 *)addr;

// //       printf("[SLAB]           [ idx %d ] { addr: %p, as_ptr: %p, as_obj: {",
// //              i, addr, (void *)as_ptr);
// //       slab_obj_printer(addr);
// //       printf(" } }\n");
// //     }
// //   }

// //   printf("[SLAB] print_kmem_cache end\n");
// // }

// void
// print_kmem_cache(struct kmem_cache *cache, void (*slab_obj_printer)(void *))
// {
//   if(cache == 0){
//     printf("[SLAB] kmem_cache { name: (null), object_size: 0, at: 0x0000000000000000, in_cache_obj: 0 }\n");
//     printf("[SLAB] print_kmem_cache end\n");
//     return;
//   }

//   acquire(&cache->lock);

//   printf("[SLAB] kmem_cache { name: %s, object_size: %d, at: %p, in_cache_obj: %d }\n",
//          cache->name, cache->object_size, cache, cache->in_cache_obj);

//   printf("[SLAB]    [ partial slabs ]\n");

//   for(struct slab *s = cache->partial; s != 0; s = s->nxt){
//     printf("[SLAB]        [ slab %p ] { freelist: %p, nxt: %p, max_objs: %d }\n",
//            s, s->freelist, s->nxt, s->max_objs);

//     if(slab_obj_printer == 0)
//       continue;

//     for(uint i = 0; i < s->max_objs; i++){
//       void *addr = slab_obj_at(s, i);
//       uint64 as_ptr = *(uint64 *)addr;

//       printf("[SLAB]           [ idx %d ] { addr: %p, as_ptr: %p, as_obj: {",
//              i, addr, (void *)as_ptr);
//       slab_obj_printer(addr);
//       printf("} }\n");
//     }
//   }

//   printf("[SLAB] print_kmem_cache end\n");

//   release(&cache->lock);
// }

// struct kmem_cache *kmem_cache_create(char *name, uint object_size)
// {
//   // Allocate one page (slab allocator control structure lives in its own page).
//   struct kmem_cache *cache = (struct kmem_cache *)kalloc();
//   if(cache == 0)
//     return 0;

//   // Ensure deterministic initial state.
//   memset(cache, 0, PGSIZE);

//   // Setup metadata.
//   safestrcpy(cache->name, name, MP2_CACHE_MAX_NAME);
//   cache->object_size = object_size;

//   // Make sure freelist pointer load/store is safe.
//   uint minsz = sizeof(void *);
//   cache->slot_size = align_up(object_size < minsz ? minsz : object_size, 8);
//   cache->max_objs = (PGSIZE > sizeof(struct slab))
//                       ? (PGSIZE - sizeof(struct slab)) / cache->slot_size
//                       : 0;

//   cache->in_cache_obj = 0; // basic part: no internal fragmentation optimization

//   cache->partial = 0;
//   cache->full = 0;
//   cache->partial_cnt = 0;

//   initlock(&cache->lock, cache->name);

//   printf(
//       "[SLAB] New kmem_cache (name: %s, object size: %d bytes, at: %p, max objects per slab: %d, support in cache obj: %d) is created\n",
//       cache->name, cache->object_size, cache, cache->max_objs, cache->in_cache_obj);

//   return cache;
// }

// void *kmem_cache_alloc(struct kmem_cache *cache)
// {
//   if(cache == 0)
//     return 0;

//   printf("[SLAB] Alloc request on cache %s\n", cache->name);

//   struct slab *new_slab = 0;
//   void *obj = 0;
//   struct slab *s = 0;

//   // Fast path: try to allocate from an existing partial slab.
//   acquire(&cache->lock);
//   s = cache->partial;
//   if(s == 0){
//     release(&cache->lock);

//     // Need a new slab (allocate outside cache lock to avoid lock-order issues).
//     new_slab = slab_new(cache);
//     if(new_slab == 0)
//       return 0;
//     printf("[SLAB] A new slab %p (%s) is allocated\n", new_slab, cache->name);

//     // Insert into partial list.
//     acquire(&cache->lock);
//     new_slab->nxt = cache->partial;
//     cache->partial = new_slab;
//     cache->partial_cnt++;
//     s = new_slab;
//   }

//   // At this point, s should be a partial slab with at least one free object.
//   obj = s->freelist;
//   if(obj == 0){
//     // Defensive: if head slab is unexpectedly full, remove it and retry.
//     // (This should not happen in the intended control flow.)
//     if(list_remove_slab(&cache->partial, s))
//       cache->partial_cnt--;
//     s->nxt = cache->full;
//     cache->full = s;
//     release(&cache->lock);

//     // Retry once.
//     return kmem_cache_alloc(cache);
//   }

//   s->freelist = get_obj_next(obj);
//   s->inuse++;

//   // If slab becomes full, move it from partial -> full.
//   if(s->inuse == s->max_objs){
//     if(list_remove_slab(&cache->partial, s))
//       cache->partial_cnt--;
//     s->nxt = cache->full;
//     cache->full = s;
//   }

//   release(&cache->lock);

//   // Initialize object (basic requirement).
//   memset(obj, 0, cache->object_size);

//   printf("[SLAB] Object %p in slab %p (%s) is allocated and initialized\n", obj,
//          obj_to_slab(obj), cache->name);

//   return obj;
// }

// void kmem_cache_free(struct kmem_cache *cache, void *obj)
// {
//   if(cache == 0 || obj == 0)
//     return;

//   struct slab *s = obj_to_slab(obj);

//   printf("[SLAB] Free %p in slab %p (%s)\n", obj, s, cache->name);

//   struct slab *to_free = 0;
//   int need_free_msg = 0;

//   acquire(&cache->lock);

//   // If the slab is currently full, it's on the full list; move it back to partial.
//   if(s->inuse == s->max_objs){
//     // Remove from full list.
//     list_remove_slab(&cache->full, s);

//     // Insert into partial list.
//     s->nxt = cache->partial;
//     cache->partial = s;
//     cache->partial_cnt++;
//   }

//   // Push the object back to the slab freelist.
//   set_obj_next(obj, s->freelist);
//   s->freelist = obj;

//   if(s->inuse > 0)
//     s->inuse--;

//   // Memory collection: if we have too many partial slabs and this slab becomes
//   // empty, return this whole slab page back to kalloc().
//   if(s->inuse == 0 && cache->partial_cnt > MP2_MIN_AVAIL_SLAB){
//     if(list_remove_slab(&cache->partial, s)){
//       cache->partial_cnt--;
//       to_free = s;
//       need_free_msg = 1;
//     }
//   }

//   release(&cache->lock);

//   if(need_free_msg){
//     printf("[SLAB] Slab %p (%s) is freed due to save memory\n", to_free,
//            cache->name);
//     kfree((void *)to_free);
//   }

//   printf("[SLAB] End of free\n");
// }

// // void kmem_cache_destroy(struct kmem_cache *cache)
// // {
// //   // This function is not tested, but we provide a best-effort cleanup.
// //   if(cache == 0)
// //     return;

// //   struct slab *s;

// //   // Free partial slabs.
// //   s = cache->partial;
// //   while(s){
// //     struct slab *next = s->nxt;
// //     kfree((void *)s);
// //     s = next;
// //   }

// //   // Free full slabs (if we tracked them).
// //   s = cache->full;
// //   while(s){
// //     struct slab *next = s->nxt;
// //     kfree((void *)s);
// //     s = next;
// //   }

// //   // Free the cache page itself.
// //   kfree((void *)cache);
// // }  
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "slab.h"

static inline uint
align_up(uint x, uint a)
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

static inline void *
slab_obj_base(struct slab *s)
{
  return (void *)((char *)s + sizeof(struct slab));
}

static inline void *
slab_obj_at(struct slab *s, uint idx)
{
  return (void *)((char *)slab_obj_base(s) + (uint64)idx * s->slot_size);
}

static int
list_remove_slab(struct slab **head, struct slab *target)
{
  struct slab **pp = head;
  while(*pp && *pp != target)
    pp = &(*pp)->nxt;
  if(*pp == 0)
    return 0;
  *pp = target->nxt;
  target->nxt = 0;
  return 1;
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
  s->nxt = 0;
  s->inuse = 0;
  s->max_objs = cache->max_objs;
  s->slot_size = cache->slot_size;
  s->reserved = 0;

  if(s->max_objs == 0)
    return s;

  for(uint i = 0; i < s->max_objs; i++){
    void *cur = slab_obj_at(s, i);
    void *nxt = (i + 1 < s->max_objs) ? slab_obj_at(s, i + 1) : 0;
    set_obj_next(cur, nxt);
  }
  s->freelist = slab_obj_at(s, 0);

  return s;
}

void
print_kmem_cache(struct kmem_cache *cache, void (*slab_obj_printer)(void *))
{
  if(cache == 0){
    printf("[SLAB] kmem_cache { name: (null), object_size: 0, at: 0x0000000000000000, in_cache_obj: 0 }\n");
    printf("[SLAB] print_kmem_cache end\n");
    return;
  }

  acquire(&cache->lock);

  printf("[SLAB] kmem_cache { name: %s, object_size: %d, at: %p, in_cache_obj: %d }\n",
         cache->name, cache->object_size, cache, cache->in_cache_obj);

  printf("[SLAB]    [ partial slabs ]\n");

  for(struct slab *s = cache->partial; s != 0; s = s->nxt){
    printf("[SLAB]        [ slab %p ] { freelist: %p, nxt: %p, max_objs: %d }\n",
           s, s->freelist, s->nxt, s->max_objs);

    if(slab_obj_printer == 0)
      continue;

    for(uint i = 0; i < s->max_objs; i++){
      void *addr = slab_obj_at(s, i);
      uint64 as_ptr = *(uint64 *)addr;

      printf("[SLAB]           [ idx %d ] { addr: %p, as_ptr: %p, as_obj: {",
             i, addr, (void *)as_ptr);
      slab_obj_printer(addr);
      printf("} }\n");
    }
  }

  printf("[SLAB] print_kmem_cache end\n");

  release(&cache->lock);
}

struct kmem_cache *
kmem_cache_create(char *name, uint object_size)
{
  struct kmem_cache *cache = (struct kmem_cache *)kalloc();
  if(cache == 0)
    return 0;

  memset(cache, 0, PGSIZE);

  safestrcpy(cache->name, name, MP2_CACHE_MAX_NAME);
  cache->object_size = object_size;

  uint minsz = sizeof(void *);
  cache->slot_size = align_up(object_size < minsz ? minsz : object_size, 8);
  cache->max_objs = (PGSIZE > sizeof(struct slab))
                      ? (PGSIZE - sizeof(struct slab)) / cache->slot_size
                      : 0;

  cache->in_cache_obj = 0;
  cache->partial = 0;
  cache->full = 0;
  cache->partial_cnt = 0;

  initlock(&cache->lock, cache->name);

  printf("[SLAB] New kmem_cache (name: %s, object size: %d bytes, at: %p, max objects per slab: %d, support in cache obj: %d) is created\n",
         cache->name, cache->object_size, cache, cache->max_objs, cache->in_cache_obj);

  return cache;
}

void *
kmem_cache_alloc(struct kmem_cache *cache)
{
  if(cache == 0)
    return 0;

  acquire(&cache->lock);

  printf("[SLAB] Alloc request on cache %s\n", cache->name);

  struct slab *s = cache->partial;

  if(s == 0){
    s = slab_new(cache);
    if(s == 0){
      release(&cache->lock);
      return 0;
    }

    printf("[SLAB] A new slab %p (%s) is allocated\n", s, cache->name);

    s->nxt = cache->partial;
    cache->partial = s;
    cache->partial_cnt++;
  }

  void *obj = s->freelist;

  if(obj == 0){
    if(list_remove_slab(&cache->partial, s))
      cache->partial_cnt--;
    s->nxt = cache->full;
    cache->full = s;
    release(&cache->lock);
    return kmem_cache_alloc(cache);
  }

  s->freelist = get_obj_next(obj);
  s->inuse++;

  if(s->inuse == s->max_objs){
    if(list_remove_slab(&cache->partial, s))
      cache->partial_cnt--;
    s->nxt = cache->full;
    cache->full = s;
  }

  memset(obj, 0, cache->object_size);

  printf("[SLAB] Object %p in slab %p (%s) is allocated and initialized\n",
         obj, obj_to_slab(obj), cache->name);

  release(&cache->lock);

  return obj;
}

void
kmem_cache_free(struct kmem_cache *cache, void *obj)
{
  if(cache == 0 || obj == 0)
    return;

  acquire(&cache->lock);

  struct slab *s = obj_to_slab(obj);

  printf("[SLAB] Free %p in slab %p (%s)\n", obj, s, cache->name);

  if(s->inuse == s->max_objs){
    list_remove_slab(&cache->full, s);
    s->nxt = cache->partial;
    cache->partial = s;
    cache->partial_cnt++;
  }

  set_obj_next(obj, s->freelist);
  s->freelist = obj;

  if(s->inuse > 0)
    s->inuse--;

  if(s->inuse == 0 && cache->partial_cnt > MP2_MIN_AVAIL_SLAB){
    if(list_remove_slab(&cache->partial, s)){
      cache->partial_cnt--;
      printf("[SLAB] Slab %p (%s) is freed due to save memory\n", s, cache->name);
      kfree((void *)s);
    }
  }

  printf("[SLAB] End of free\n");

  release(&cache->lock);
}

void
kmem_cache_destroy(struct kmem_cache *cache)
{
  if(cache == 0)
    return;

  acquire(&cache->lock);

  struct slab *s, *next;

  s = cache->partial;
  while(s){
    next = s->nxt;
    kfree((void *)s);
    s = next;
  }

  s = cache->full;
  while(s){
    next = s->nxt;
    kfree((void *)s);
    s = next;
  }

  release(&cache->lock);

  kfree((void *)cache);
}