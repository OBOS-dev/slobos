/*
 * src/slobos.h
 *
 * Copyright (c) 2024 Omar Berrow
 */

#pragma once

// NOTE: SLOBOS has no resemblence to the Linux SLUB, SLOB, or SLAB allocators.

#include <stdint.h>
#include <stddef.h>

#include <slobos_defines.h>

// Must be a power of two
#ifndef CACHE_SIZE_DEFAULT
#   define CACHE_SIZE_DEFAULT 4096
#endif

#ifndef slobos_popcount
#   error Define slobos_popcount. With GCC C extentions, this looks like __builtin_popcountll.
#endif

// next two are assumed to operate on 64-bit numbers, and if they don't
// bad stuff might happen.

#ifndef slobos_bsf
#   error Define slobos_bsf. With GCC C extensions, this looks like __builtin_clzll.
#endif

#ifndef slobos_bsr
#   error Define slobos_bsr. With GCC C extensions, this looks like __builtin_ctzll.
#endif

#ifndef SLOBOS_ALIGN
#   error Define SLOBOS_ALIGN
#   define SLOBOS_ALIGN(struct_, align_to)
#endif

// To be defined by user.

// map_hnd is an optional value that can be used to differentiate between different slab_allocator_t while mapping
// For example, one can use this to differentiate between a non-paged pool allocator,
// and a paged-pool allocator.

// Read-Write No-Execute
// metadata is stored as a 4-bit value.
extern void  *slobos_map(uintptr_t map_hnd, uint8_t* metadata, size_t size);
extern void   slobos_unmap(uintptr_t map_hnd, uint8_t metadata, void* base, size_t size);
extern size_t slobos_pgsize();

typedef struct slobos_allocator *slobos_allocator_t;

// Returns sizeof(struct slobos_allocator)
size_t slobos_allocator_size();

// NOTE: This is not thread-safe!
// Thread-safety must be added on top of these APIs.

// Returns zero on success, one on failure.
//     maxSize: The maximum slobos size; must be a power of two greater than 32, and less than cacheSize. Hard limit is 0x40000.
//   cacheSize: The cache size; must be a power of two greater than maxSize. If zero, CACHE_SIZE_DEFAULT is assumed. Hard limit is 0x40000
//     map_hnd: See slobos_map
int slobos_init(slobos_allocator_t state, size_t maxSize, size_t cacheSize, uintptr_t map_hnd);
// map_hnd: See slobos_map
int slobos_set_map_hnd(slobos_allocator_t state, uintptr_t map_hnd);

void* slobos_alloc(slobos_allocator_t state, size_t size);
void* slobos_calloc(slobos_allocator_t state, size_t objs, size_t szObjs);
void* slobos_realloc(slobos_allocator_t state, void* blk, size_t newsize);
void  slobos_free(slobos_allocator_t state, void* blk);
void  slobos_getsize(slobos_allocator_t state, void* blk, size_t* sz);
