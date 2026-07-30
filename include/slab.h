#ifndef SHADOWBOX_SLAB_H
#define SHADOWBOX_SLAB_H

#include "types.h"
#include "spinlock.h"

#define SLAB_OBJ_MAX_SIZE 4096
#define SLAB_DEFAULT_ORDER 0

#define SLUB_HWCACHE_ALIGN 0x0001
#define SLUB_POISON        0x0002
#define SLUB_RED_ZONE      0x0004
#define SLUB_DEBUG         0x0008

/*
 * slab_page_t - A page of slab objects
 * @next, @prev: Linked list pointers
 * @s_mem:       First object address
 * @inuse:       Number of allocated objects
 * @objects:     Total number of objects in page
 * @frozen:      Page is frozen for per-CPU use
 * @timestamp:   Last used timestamp
 */
typedef struct slab_page {
    struct slab_page *next;
    struct slab_page *prev;
    void *s_mem;
    int inuse;
    int objects;
    int frozen;
    uint64_t timestamp;
} slab_page_t;

/*
 * slab_cache_t - SLUB allocator cache descriptor
 * @name:          Cache name
 * @object_size:   Size of each object
 * @align:         Required alignment
 * @size:          Aligned object size
 * @offset:        Free pointer offset
 * @flags:         SLUB flags (SLUB_*)
 * @slabs_partial: Partially filled slabs
 * @slabs_free:    Completely free slabs
 * @allocs, @frees, @errors: Statistics
 * @lock:          Cache lock
 * @min_partial:   Minimum partial slabs to keep
 * @gfporder:      Page allocation order
 * @objects:       Objects per slab
 * @ctor, @dtor:   Constructor/destructor callbacks
 */
typedef struct slab_cache {
    const char *name;
    size_t object_size;
    size_t align;
    size_t size;
    size_t offset;
    unsigned long flags;

    slab_page_t *slabs_partial;
    slab_page_t *slabs_free;

    uint64_t allocs;
    uint64_t frees;
    uint64_t errors;

    spinlock_t lock;

    unsigned int min_partial;
    unsigned int gfporder;
    unsigned int objects;

    void (*ctor)(void *obj);
    void (*dtor)(void *obj);
} slab_cache_t;

/*
 * slab_init - Initialize SLUB allocator
 */
void slab_init(void);

/*
 * slab_create_cache - Create a new slab cache
 * @name:        Cache name
 * @object_size: Object size
 * @align:       Required alignment
 * @flags:       SLUB flags
 * Returns:      New cache, or NULL on failure
 */
slab_cache_t* slab_create_cache(const char *name, size_t object_size, size_t align, unsigned long flags);

/*
 * slab_destroy_cache - Destroy a slab cache
 * @cache: Cache to destroy
 */
void slab_destroy_cache(slab_cache_t *cache);

/*
 * slab_alloc - Allocate an object from a slab cache
 * @cache: Cache to allocate from
 * Returns: Allocated object, or NULL
 */
void* slab_alloc(slab_cache_t *cache);

/*
 * slab_free - Free an object back to its slab cache
 * @cache: Cache to return to
 * @obj:   Object to free
 */
void slab_free(slab_cache_t *cache, void *obj);

/*
 * slab_cache_stats - Print cache statistics
 * @cache: Cache to display stats for
 */
void slab_cache_stats(slab_cache_t *cache);

/*
 * kmem_cache_alloc - Allocate from slab cache (alias)
 * @cache: Cache to allocate from
 * Returns: Allocated object, or NULL
 */
void* kmem_cache_alloc(slab_cache_t *cache);

/*
 * kmem_cache_free - Free to slab cache (alias)
 * @cache: Cache to return to
 * @obj:   Object to free
 */
void kmem_cache_free(slab_cache_t *cache, void *obj);

extern slab_cache_t *kmalloc_caches[12];

/*
 * kmalloc - Allocate kernel memory (general purpose)
 * @size: Size in bytes
 * Returns: Allocated memory, or NULL
 */
void* kmalloc(size_t size);

/*
 * kfree - Free kernel memory
 * @ptr: Pointer to free
 */
void kfree(void *ptr);

#endif
