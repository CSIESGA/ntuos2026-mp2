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

// static inline uint
// align_up(uint x, uint a)
// {
//   return (x + a - 1) & ~(a - 1);
// }

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

//   memset(page, 0, PGSIZE);

//   struct slab *s = (struct slab *)page;
//   s->freelist = 0;
//   s->nxt = 0;
//   s->inuse = 0;
//   s->max_objs = cache->max_objs;
//   s->slot_size = cache->slot_size;
//   s->reserved = 0;

//   if(s->max_objs == 0)
//     return s;

//   for(uint i = 0; i < s->max_objs; i++){
//     void *cur = slab_obj_at(s, i);
//     void *nxt = (i + 1 < s->max_objs) ? slab_obj_at(s, i + 1) : 0;
//     set_obj_next(cur, nxt);
//   }
//   s->freelist = slab_obj_at(s, 0);

//   return s;
// }

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

// struct kmem_cache*
// kmem_cache_create(char *name, uint object_size)
// {
//   struct kmem_cache *cache = (struct kmem_cache *)kalloc();
//   if(cache == 0)
//     return 0;

//   memset(cache, 0, PGSIZE);

//   safestrcpy(cache->name, name, MP2_CACHE_MAX_NAME);
//   cache->object_size = object_size;

//   uint minsz = sizeof(void *);
//   cache->slot_size = align_up(object_size < minsz ? minsz : object_size, 8);
//   cache->max_objs = (PGSIZE > sizeof(struct slab))
//                       ? (PGSIZE - sizeof(struct slab)) / cache->slot_size
//                       : 0;

//   cache->in_cache_obj = 0;
//   cache->partial = 0;
//   cache->full = 0;
//   cache->partial_cnt = 0;

//   initlock(&cache->lock, cache->name);

//   printf("[SLAB] New kmem_cache (name: %s, object size: %d bytes, at: %p, max objects per slab: %d, support in cache obj: %d) is created\n",
//          cache->name, cache->object_size, cache, cache->max_objs, cache->in_cache_obj);

//   return cache;
// }

// void *
// kmem_cache_alloc(struct kmem_cache *cache)
// {
//   if(cache == 0)
//     return 0;

//   acquire(&cache->lock);

//   printf("[SLAB] Alloc request on cache %s\n", cache->name);

//   struct slab *s = cache->partial;

//   if(s == 0){
//     s = slab_new(cache);
//     if(s == 0){
//       release(&cache->lock);
//       return 0;
//     }

//     printf("[SLAB] A new slab %p (%s) is allocated\n", s, cache->name);

//     s->nxt = cache->partial;
//     cache->partial = s;
//     cache->partial_cnt++;
//   }

//   void *obj = s->freelist;

//   if(obj == 0){
//     if(list_remove_slab(&cache->partial, s))
//       cache->partial_cnt--;
//     s->nxt = cache->full;
//     cache->full = s;
//     release(&cache->lock);
//     return kmem_cache_alloc(cache);
//   }

//   s->freelist = get_obj_next(obj);
//   s->inuse++;

//   if(s->inuse == s->max_objs){
//     if(list_remove_slab(&cache->partial, s))
//       cache->partial_cnt--;
//     s->nxt = cache->full;
//     cache->full = s;
//   }

//   memset(obj, 0, cache->object_size);

//   printf("[SLAB] Object %p in slab %p (%s) is allocated and initialized\n",
//          obj, obj_to_slab(obj), cache->name);

//   release(&cache->lock);

//   return obj;
// }

// void
// kmem_cache_free(struct kmem_cache *cache, void *obj)
// {
//   if(cache == 0 || obj == 0)
//     return;

//   acquire(&cache->lock);

//   struct slab *s = obj_to_slab(obj);

//   printf("[SLAB] Free %p in slab %p (%s)\n", obj, s, cache->name);

//   if(s->inuse == s->max_objs){
//     list_remove_slab(&cache->full, s);
//     s->nxt = cache->partial;
//     cache->partial = s;
//     cache->partial_cnt++;
//   }

//   set_obj_next(obj, s->freelist);
//   s->freelist = obj;

//   if(s->inuse > 0)
//     s->inuse--;

//   if(s->inuse == 0 && cache->partial_cnt > MP2_MIN_AVAIL_SLAB){
//     if(list_remove_slab(&cache->partial, s)){
//       cache->partial_cnt--;
//       printf("[SLAB] Slab %p (%s) is freed due to save memory\n", s, cache->name);
//       kfree((void *)s);
//     }
//   }

//   printf("[SLAB] End of free\n");

//   release(&cache->lock);
// }

// void
// kmem_cache_destroy(struct kmem_cache *cache)
// {
//   if(cache == 0)
//     return;

//   acquire(&cache->lock);

//   struct slab *s, *next;

//   s = cache->partial;
//   while(s){
//     next = s->nxt;
//     kfree((void *)s);
//     s = next;
//   }

//   s = cache->full;
//   while(s){
//     next = s->nxt;
//     kfree((void *)s);
//     s = next;
//   }

//   release(&cache->lock);

//   kfree((void *)cache);
// }
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
  return (void *)((char *)slab_obj_base(s) + (uint64)idx * (uint)s->slot_size);
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
  s->max_objs = (ushort)cache->max_objs;
  s->slot_size = (ushort)cache->slot_size;
  s->reserved = 0;

  if(s->max_objs == 0)
    return s;

  for(uint i = 0; i < (uint)s->max_objs; i++){
    void *cur = slab_obj_at(s, i);
    void *nxt = (i + 1 < (uint)s->max_objs) ? slab_obj_at(s, i + 1) : 0;
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

  // Use list_for_each_entry as required by the bonus.
  if(!list_empty(&cache->partial)){
    struct slab *s;
    list_for_each_entry(s, &cache->partial, list){
      void *nxt = (void *)0;
      if(s->list.next != &cache->partial){
        struct slab *ns = list_entry(s->list.next, struct slab, list);
        nxt = (void *)ns;
      }

      printf("[SLAB]        [ slab %p ] { freelist: %p, nxt: %p, max_objs: %d }\n",
             s, s->freelist, nxt, (uint)s->max_objs);

      if(slab_obj_printer == 0)
        continue;

      for(uint i = 0; i < (uint)s->max_objs; i++){
        void *addr = slab_obj_at(s, i);
        uint64 as_ptr = *(uint64 *)addr;

        printf("[SLAB]           [ idx %d ] { addr: %p, as_ptr: %p, as_obj: {",
               i, addr, (void *)as_ptr);
        slab_obj_printer(addr);
        printf("} }\n");
      }
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
  cache->partial_cnt = 0;
  INIT_LIST_HEAD(&cache->partial);
  INIT_LIST_HEAD(&cache->full);

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

  for(;;){
    if(list_empty(&cache->partial)){
      struct slab *snew = slab_new(cache);
      if(snew == 0){
        release(&cache->lock);
        return 0;
      }

      printf("[SLAB] A new slab %p (%s) is allocated\n", snew, cache->name);

      // Insert to the head to preserve the same behavior as the old pointer-based version.
      list_add(&snew->list, &cache->partial);
      cache->partial_cnt++;
    }

    struct slab *s = list_first_entry(&cache->partial, struct slab, list);
    void *obj = s->freelist;

    if(obj == 0){
      // Defensive path: move this slab to full and retry.
      list_del_init(&s->list);
      if(cache->partial_cnt > 0)
        cache->partial_cnt--;
      list_add(&s->list, &cache->full);
      continue;
    }

    s->freelist = get_obj_next(obj);
    s->inuse++;

    if(s->inuse == s->max_objs){
      list_del_init(&s->list);
      if(cache->partial_cnt > 0)
        cache->partial_cnt--;
      list_add(&s->list, &cache->full);
    }

    memset(obj, 0, cache->object_size);

    printf("[SLAB] Object %p in slab %p (%s) is allocated and initialized\n",
           obj, obj_to_slab(obj), cache->name);

    release(&cache->lock);
    return obj;
  }
}

void
kmem_cache_free(struct kmem_cache *cache, void *obj)
{
  if(cache == 0 || obj == 0)
    return;

  acquire(&cache->lock);

  struct slab *s = obj_to_slab(obj);
  struct slab *to_free = 0;

  printf("[SLAB] Free %p in slab %p (%s)\n", obj, s, cache->name);

  if(s->inuse == s->max_objs){
    list_del_init(&s->list);
    list_add(&s->list, &cache->partial);
    cache->partial_cnt++;
  }

  set_obj_next(obj, s->freelist);
  s->freelist = obj;

  if(s->inuse > 0)
    s->inuse--;

  if(s->inuse == 0 && cache->partial_cnt > MP2_MIN_AVAIL_SLAB){
    list_del_init(&s->list);
    cache->partial_cnt--;
    to_free = s;
    printf("[SLAB] Slab %p (%s) is freed due to save memory\n", s, cache->name);
  }

  printf("[SLAB] End of free\n");

  release(&cache->lock);

  if(to_free)
    kfree((void *)to_free);
}

void
kmem_cache_destroy(struct kmem_cache *cache)
{
  if(cache == 0)
    return;

  acquire(&cache->lock);

  if(!list_empty(&cache->partial)){
    struct slab *s, *tmp;
    list_for_each_entry_safe(s, tmp, &cache->partial, list){
      list_del_init(&s->list);
      kfree((void *)s);
    }
  }

  if(!list_empty(&cache->full)){
    struct slab *s, *tmp;
    list_for_each_entry_safe(s, tmp, &cache->full, list){
      list_del_init(&s->list);
      kfree((void *)s);
    }
  }

  release(&cache->lock);

  kfree((void *)cache);
}