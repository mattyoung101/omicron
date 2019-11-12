#pragma once

#include <stdint.h>
#include <stdlib.h>
#define POOL_REF_T char *
#define FRAME_POOL "FramePool"

/*
 * Allocation Pool idea: essentially, we keep track of a bunch of mallocs in a linked list and then you can free
 * them all at once just by going apool_free(pool_id)
 */

/**
 * Create a new allocation pool (or Australian Labour Party, haha).
 * Internally, the new pool would be added to the pools list.
 * @param id the unique ID of the pool
 */
void alp_create(POOL_REF_T id);
/**
 * Allocates a certain number of bytes with malloc and pushes the allocation to the given allocation pool.
 * @param id the ID of the pool
 * @param bytes the number of bytes to allocate
 * @return the pointer returned by malloc()
 */
void *alp_malloc(POOL_REF_T id, size_t bytes);
/**
 * Allocates a certain number of bytes with calloc and pushes the allocation to the given allocation pool.
 * @param id the ID of the pool
 * @param nmemb the number of items to allocate
 * @param size the size of each member
 * @return the pointer returned by calloc()
 */
void *alp_calloc(POOL_REF_T id, size_t nmemb, size_t size);
/**
 * Destroys all allocations owned by the given pool, but does not destroy the pool itself.
 * @param id the ID of the pool
 */
void alp_free(POOL_REF_T id);

/**
 * Destroys a pool and all the allocations it contains
 * @param id the ID of the pool
 */
void alp_free_pool(POOL_REF_T id);

