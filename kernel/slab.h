// // #pragma once

// // #include "spinlock.h"

// // // struct run {
// // //   struct run *next;
// // // };

// // /**
// //  * struct slab - Represents a slab in the slab allocator.
// //  * @freelist: Linked list of free objects.
// //  */
// // struct slab {
// //   // TODO: Choose the type of freelist from
// //   //    1. void **
// //   //    2. struct run *
// //   // <ptr> freelist;             // Linked list of free objects

// //   // TODO: Design how to link the slabs
// //   // ...

// //   // TODO: you can add other members
// //   // ...
// // };

// // /**
// //  * struct kmem_cache - Represents a cache of slabs.
// //  * @name: Cache name (e.g., "file").
// //  * @object_size: Size of a single object.
// //  * @lock: Lock for cache management.
// //  */
// // struct kmem_cache {
// //   char name[MP2_CACHE_MAX_NAME]; // Cache name (e.g., "file")
// //   uint object_size;              // Size of a single object
// //   struct spinlock lock;          // Lock for cache management

// //   // TODO: Add slab list(s)
// //   // <TYPE> full     // Completely allocated slabs (Optional)
// //   // <TYPE> partial  // Partially allocated or empty slabs
// // };

// // /**
// //  * kmem_cache_create - Create a new slab cache.
// //  * @name: The name of the cache.
// //  * @object_size: The size of each object in the cache.
// //  *
// //  * Return: A pointer to the new cache.
// //  */
// // struct kmem_cache *kmem_cache_create(char *name, uint object_size);

// // /**
// //  * kmem_cache_destroy - Destroy a slab cache.
// //  * @cache: The cache to be destroyed.
// //  */
// // void kmem_cache_destroy(struct kmem_cache *cache);

// // /**
// //  * kmem_cache_alloc - Allocate an object from a slab cache.
// //  * @cache: The cache to allocate from.
// //  *
// //  * Return: A pointer to the allocated object.
// //  */
// // void *kmem_cache_alloc(struct kmem_cache *cache);

// // /**
// //  * kmem_cache_free - Free an object back to its slab cache.
// //  * @cache: The cache to free to.
// //  * @obj: The object to free.
// //  */
// // void kmem_cache_free(struct kmem_cache *cache, void *obj);

// // /**
// //  * print_kmem_cache - Print the details of a kmem_cache.
// //  * @cache: The cache to print.
// //  * @print_fn: Function to print each object in the cache. If NULL (0) is given,
// //  * will skip object printing part.
// //  */
// // void print_kmem_cache(struct kmem_cache *cache, void (*print_fn)(void *));


// #pragma once

// #include "types.h"
// #include "spinlock.h"

// // struct run {
// //   struct run *next;
// // };

// /**
//  * struct slab - Represents a slab in the slab allocator.
//  * @freelist: Linked list of free objects.
//  */
// struct slab {
//   // Head pointer of the freelist. Each free object stores the next pointer in
//   // its first machine word (8 bytes on xv6-riscv).
//   void *freelist;

//   // Next slab pointer used to link slabs inside a kmem_cache.
//   //
//   // Note: For the basic (non-bonus) implementation, we only maintain two
//   // lists:
//   //   * cache->partial : slabs that have at least one free object
//   //   * cache->full    : slabs that are fully allocated
//   struct slab *nxt;

//   // Bookkeeping (not printed by print_kmem_cache).
//   uint inuse;    // number of allocated objects in this slab
//   uint max_objs; // total number of objects in this slab

//   // The stride between two objects inside this slab.
//   // We keep it 8-byte aligned so we can safely read the first 8 bytes as_ptr.
//   uint slot_size;

//   // Padding/reserved to keep the slab header 32 bytes (matches the sample
//   // layout where the first object starts at slab_base + 0x20).
//   uint reserved;
// };

// /**
//  * struct kmem_cache - Represents a cache of slabs.
//  * @name: Cache name (e.g., "file").
//  * @object_size: Size of a single object.
//  * @lock: Lock for cache management.
//  */
// struct kmem_cache {
//   char name[MP2_CACHE_MAX_NAME]; // Cache name (e.g., "file")
//   uint object_size;              // Size of a single object
//   struct spinlock lock;          // Lock for cache management

//   // Slab lists
//   struct slab *partial; // Partially allocated or empty slabs
//   struct slab *full;    // Completely allocated slabs (not printed)

//   // Cached parameters
//   uint slot_size;    // aligned stride between objects
//   uint max_objs;     // max objects per slab page
//   uint in_cache_obj; // 0 for basic part; bonus may set >0

//   // Bookkeeping
//   uint partial_cnt; // number of slabs in the partial list
// };

// /**
//  * kmem_cache_create - Create a new slab cache.
//  * @name: The name of the cache.
//  * @object_size: The size of each object in the cache.
//  *
//  * Return: A pointer to the new cache.
//  */
// struct kmem_cache *kmem_cache_create(char *name, uint object_size);

// /**
//  * kmem_cache_destroy - Destroy a slab cache.
//  * @cache: The cache to be destroyed.
//  */
// void kmem_cache_destroy(struct kmem_cache *cache);

// /**
//  * kmem_cache_alloc - Allocate an object from a slab cache.
//  * @cache: The cache to allocate from.
//  *
//  * Return: A pointer to the allocated object.
//  */
// void *kmem_cache_alloc(struct kmem_cache *cache);

// /**
//  * kmem_cache_free - Free an object back to its slab cache.
//  * @cache: The cache to free to.
//  * @obj: The object to free.
//  */
// void kmem_cache_free(struct kmem_cache *cache, void *obj);

// /**
//  * print_kmem_cache - Print the details of a kmem_cache.
//  * @cache: The cache to print.
//  * @print_fn: Function to print each object in the cache. If NULL (0) is given,
//  * will skip object printing part.
//  */
// void print_kmem_cache(struct kmem_cache *cache, void (*print_fn)(void *));
#pragma once

#include "types.h"
#include "spinlock.h"
#include "list.h"

/**
 * struct slab - Represents a slab in the slab allocator.
 * @freelist: Linked list of free objects.
 *
 * Note:
 * We use Linux-style intrusive list via @list to manage slab lists.
 * To preserve the same object packing for the basic tests, we keep this
 * structure 32 bytes wide.
 */
struct slab {
  // Head pointer of the freelist. Each free object stores the next pointer in
  // its first machine word (8 bytes on xv6-riscv).
  void *freelist;            // 8 bytes

  // Intrusive list node for partial/full lists.
  struct list_head list;     // 16 bytes

  // Bookkeeping (not printed by print_kmem_cache).
  ushort inuse;              // 2 bytes
  ushort max_objs;           // 2 bytes
  ushort slot_size;          // 2 bytes
  ushort reserved;           // 2 bytes
};

/**
 * struct kmem_cache - Represents a cache of slabs.
 * @name: Cache name (e.g., "file").
 * @object_size: Size of a single object.
 * @lock: Lock for cache management.
 */
struct kmem_cache {
  char name[MP2_CACHE_MAX_NAME]; // Cache name (e.g., "file")
  uint object_size;              // Size of a single object
  struct spinlock lock;          // Lock for cache management

  // Slab lists managed by Linux-style intrusive list API.
  struct list_head partial;      // Partially allocated or empty slabs
  struct list_head full;         // Completely allocated slabs (not printed)

  // Cached parameters
  uint slot_size;                // aligned stride between objects
  uint max_objs;                 // max objects per slab page
  uint in_cache_obj;             // still 0 for this stage

  // Bookkeeping
  uint partial_cnt;              // number of slabs in the partial list
};

/**
 * kmem_cache_create - Create a new slab cache.
 * @name: The name of the cache.
 * @object_size: The size of each object in the cache.
 *
 * Return: A pointer to the new cache.
 */
struct kmem_cache *kmem_cache_create(char *name, uint object_size);

/**
 * kmem_cache_destroy - Destroy a slab cache.
 * @cache: The cache to be destroyed.
 */
void kmem_cache_destroy(struct kmem_cache *cache);

/**
 * kmem_cache_alloc - Allocate an object from a slab cache.
 * @cache: The cache to allocate from.
 *
 * Return: A pointer to the allocated object.
 */
void *kmem_cache_alloc(struct kmem_cache *cache);

/**
 * kmem_cache_free - Free an object back to its slab cache.
 * @cache: The cache to free to.
 * @obj: The object to free.
 */
void kmem_cache_free(struct kmem_cache *cache, void *obj);

/**
 * print_kmem_cache - Print the details of a kmem_cache.
 * @cache: The cache to print.
 * @print_fn: Function to print each object in the cache. If NULL (0) is given,
 * will skip object printing part.
 */
void print_kmem_cache(struct kmem_cache *cache, void (*print_fn)(void *));