/*
 * src/slobos.c
 *
 * Copyright (c) 2024 Omar Berrow
 */

#include <stdint.h>
#include <stddef.h>

#include "slobos.h"

#define SLOBOS_MAGIC 0x51ABC000 /* lower 8 bits reserved for the index into the cache. */

struct slobos
{
    struct slobos_cache* owner;
    uint32_t head; // freelist head.
    uint32_t magic;
    struct slobos *next, *prev;
    char data[];
} SLOBOS_ALIGN(1);
#define slobos_get_entry(slobos, entry_index, sz_slobos) (void*)(slobos->data + sz_slobos*entry_index)

struct slobos_cache
{
    struct slobos *head, *tail;
    struct slobos_allocator* owner;
};

struct slobos_allocator
{
    struct slobos_cache caches[18-5 /* bsf(0x40000) - bsf(0x20) */];
    size_t nCaches;
    size_t cacheSize;
    uintptr_t map_hnd;
};

size_t slobos_allocator_size()
{
    return sizeof(struct slobos_allocator);
}

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
int slobos_init(slobos_allocator_t state, size_t maxSize, size_t cacheSize, uintptr_t map_hnd)
{
    if (!state)
        return 1;
    if (slobos_popcount(maxSize) != 1)
        return 1;
    if (!cacheSize)
        cacheSize = CACHE_SIZE_DEFAULT;
    if (slobos_popcount(cacheSize) != 1 || cacheSize > 0x40000)
        return 1;
    if (maxSize < 32)
        return 1;
    if (cacheSize <= maxSize)
        return 1;

    state->nCaches = slobos_bsf(maxSize)-5;
    state->cacheSize = cacheSize;
    for (size_t i = 0; i < state->nCaches; i++)
        state->caches[i].owner = state;
    return slobos_set_map_hnd(state, map_hnd);
}
int slobos_set_map_hnd(slobos_allocator_t state, uintptr_t map_hnd)
{
    if (!state)
        return 1;
    state->map_hnd = map_hnd;
    return 0;
}

#ifdef __GNUC__
__attribute__((no_sanitize("address"))) __attribute__((optimize("O3")))
static void *slobos_memzero(void* dest, size_t sz)
{
    return __builtin_memset(dest, 0, sz);
}
__attribute__((no_sanitize("address"))) __attribute__((optimize("O3")))
static void *slobos_memcpy(void* dest, const void* src, size_t sz)
{
    return __builtin_memcpy(dest,src,sz);
}
#else
static void *slobos_memcpy(void* dest, const void* src, size_t sz)
{
    for (size_t i = 0; i < sz; i++)
        ((char*)dest)[i] = ((char*)src)[i];
    return dest;
}
static void *slobos_memzero(void* blk, size_t size)
{
    for (size_t i = 0; i < size; i++)
        ((char*)blk)[i] = 0;
    return blk;
}
#endif

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
struct slobos* allocate_slobos(struct slobos_cache* cache, uint8_t cache_index, size_t size)
{
    struct slobos* slobos = slobos_map(cache->owner->map_hnd, cache->owner->cacheSize);
//    printf("allocated new slobos at %p of %lu bytes\n", slobos, cache->owner->cacheSize);
    slobos_memzero(slobos, cache->owner->cacheSize);
    slobos->magic = SLOBOS_MAGIC | cache_index;
    slobos->owner = cache;
    slobos->prev = NULL;
    slobos->next = slobos->prev;

    slobos->head = 0;
    for (size_t i = 0; i < ((cache->owner->cacheSize-sizeof(struct slobos)) / size); i++)
        *(uint32_t*)slobos_get_entry(slobos, i, size) = i+1;
//    *(uint32_t*)slobos_get_entry(slobos, (cache->owner->cacheSize / size), size) = UINT32_MAX;

    return slobos;
}

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
void* slobos_alloc(slobos_allocator_t state, size_t size)
{
    if (!state || !size || size > 0x40000)
        return NULL;
    // Look for an approriate cache, then take a slobos.
    // Allocate one entry in the slobos, return it.
    // If the slobos is empty after the allocation, unlink it, and set head to UINT32_MAX

    if (slobos_popcount(size) > 1)
        size = (size_t)1 << (64-slobos_bsr(size)); // align to a slobos size
    if (size < 32)
        size = 32;

    // size is now a slobos size.

    size_t cache_index = slobos_bsf(size) - 5;
    struct slobos_cache* cache = cache_index < state->nCaches ? &state->caches[cache_index] : NULL;
    struct slobos* slobos = NULL;
    void* ret = NULL;

    if (!cache)
    {
        //printf("bruh, size=%lu\n", size);
        return NULL;
    }

    slobos = cache->head;
    if (!slobos)
        slobos = allocate_slobos(cache, cache_index, size);
    ret = slobos_get_entry(slobos, slobos->head, size);
    slobos->head = *(uint32_t*)ret /* entry->next */;
    if (slobos->head == UINT32_MAX)
    {
        // slobos has no free nodes, remove it.
        if (slobos->next)
            slobos->next->prev = slobos->prev;
        if (slobos->prev)
            slobos->prev->next = slobos->next;
        if (cache->tail == slobos)
            cache->tail = slobos->prev;
        if (cache->head == slobos)
            cache->head = slobos->next;
    }

    return ret;
}

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
void* slobos_calloc(slobos_allocator_t state, size_t objs, size_t szObjs)
{
    return slobos_memzero(slobos_alloc(state, objs*szObjs), objs*szObjs);
}

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
void* slobos_realloc(slobos_allocator_t state, void* blk, size_t newsize)
{
    if (!state)
        return NULL;
    if (!newsize)
    {
        slobos_free(state, blk);
        return NULL;
    }
    if (!blk)
        return slobos_alloc(state, newsize);
    size_t sz = 0;
    slobos_getsize(state, blk, &sz);
    if (sz == SIZE_MAX)
        return NULL;
    void* newblk = slobos_alloc(state, newsize);
    if (!newblk)
        return NULL;
    slobos_memcpy(newblk, blk, sz);
    slobos_free(state, blk);
    return newblk;
}

// Verifies if the slobos passed is valid, if so, it returns it, otherwise, it returns NULL.
static struct slobos* slobos_valid(slobos_allocator_t state, struct slobos* slobos, int* magic_valid /* boolean, 0=false, 1=true */)
{
    *magic_valid = 0;
    if ((slobos->magic & 0xffffff00) != SLOBOS_MAGIC)
        return NULL; // Invalid.
    uint8_t cache_index = slobos->magic & 0xff;
    if (cache_index > slobos->owner->owner->nCaches)
        return NULL; // uh oh....
    *magic_valid = 1;
    if (slobos->owner->owner != state)
        return NULL; // Invalid. TODO: Log violation to user?

    return slobos;
}

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
static struct slobos* slobos_from_blk(slobos_allocator_t state, void* blk)
{
    int magic_valid = 0;
    if (state->cacheSize == slobos_pgsize())
    {
        // Here, we can (probably) just return the blk & page_size to get the slobos

        return slobos_valid(state, (struct slobos*)((uintptr_t)blk & ~(state->cacheSize-1)), &magic_valid);
    }

    // TODO: Find better algorithm.
    // Find the slobos by going backwards until we find a page with the magic, or we have searched state->cacheSize bytes.

    struct slobos* curr = (struct slobos*)((uintptr_t)blk & ~(slobos_pgsize()-1));
    for (size_t nSearched = 0; nSearched < state->cacheSize; nSearched += slobos_pgsize())
    {
        if (slobos_valid(state, curr, &magic_valid))
            return curr;
        if(magic_valid)
            break; // The magic is valid, but the allocators mismatch. Abort early.
        curr = (void*)((uintptr_t)curr - slobos_pgsize()); // go down...
    }
    return NULL; // invalid block. NOTE: It is possible we page faulted by the time we get here, if the block truly is invalid.
}

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
void slobos_free(slobos_allocator_t state, void* blk)
{
    if (!state || !blk)
        return;

    // Round blk down to a multiple of the cache size to get its slobos,
    // prepend it to the free list, and if needed, add it back to the cache.

    struct slobos* slobos = slobos_from_blk(state, blk);
    if (!slobos)
        return; // invalid.

    uint8_t cache_index = slobos->magic & 0xff;

    size_t size = 1 << (cache_index+5);
    struct slobos_cache* cache = &state->caches[cache_index];

    // Do alignment.
    blk = (void*)((uintptr_t)blk & ~(size-1));

    slobos_memzero(blk, 4);
    if (slobos->head == UINT32_MAX)
    {
        // Emplace the slobos into the cache.
        if (cache->tail)
            cache->tail->next = slobos;
        if (!cache->head)
            cache->head = slobos;
        slobos->prev = cache->tail;
        cache->tail = slobos;
    }
    *(uint32_t*)blk = slobos->head;
    slobos->head = ((uintptr_t)blk - (uintptr_t)slobos) / size;
}

#ifdef __GNUC__
__attribute__((no_sanitize("address")))
#endif
void  slobos_getsize(slobos_allocator_t state, void* blk, size_t* sz)
{
    if (!sz)
        return;
    if (!state || !blk)
    {
        *sz = SIZE_MAX;
        return;
    }

    struct slobos* slobos = slobos_from_blk(state, blk);
    if (!slobos)
    {
        *sz = SIZE_MAX;
        return; // invalid.
    }

    uint8_t cache_index = slobos->magic & 0xff;

    size_t size = 1 << (cache_index+5);
    *sz = size;
    return;
}
