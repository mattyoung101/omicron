#include <stdio.h>
#include <log/log.h>
#include "alloc_pool.h"
#include "DG_dynarr.h"
#include "map.h"

DA_TYPEDEF(void*, pool_t) // the allocation list of a single pool
typedef map_t(pool_t) pool_list_t;
static pool_list_t pools = {{0}, 0, {0}}; // the list of pools

// TODO error checking to make sure the pool you're accessing exists
// in theory we wouldn't use strings, numbers instead, but the map library doesn't support numeric keys
// and strings aren't that bad I don't think

void alp_create(POOL_REF_T id){
    pool_t pool = {0};
    map_set(&pools, id, pool);
    log_trace("Creating new allocation pool: %s", id);
}

void *alp_malloc(POOL_REF_T id, size_t bytes){
    pool_t *pool = map_get(&pools, id);
    if (pool == NULL){
        log_error("No such pool: %s (allocation was not performed)", id);
        return NULL;
    }
    void *buf = malloc(bytes);
    da_add(*pool, buf);
    return buf;
}

void *alp_calloc(POOL_REF_T id, size_t nmemb, size_t size){
    pool_t *pool = map_get(&pools, id);
    if (pool == NULL){
        log_error("No such pool: %s (allocation was not performed)", id);
        return NULL;
    }
    void *buf = calloc(nmemb, size);
    da_add(*pool, buf);
    return buf;
}

void alp_register_allocation(POOL_REF_T id, void *allocation){
    pool_t *pool = map_get(&pools, id);
    if (pool == NULL){
        log_error("No such pool: %s (allocation was not performed)", id);
        return;
    }
    da_add(*pool, allocation);
}

void alp_free(POOL_REF_T id){
    pool_t *pool = map_get(&pools, id);
    if (pool == NULL){
        log_error("No such pool: %s (free will not be performed)", id);
        return;
    }
    size_t poolSize = da_count(*pool);
    for (size_t i = 0; i < poolSize; i++){
        void *allocation = da_get(*pool, i);
        printf("freeing: 0x%X\n", allocation);
        free(allocation);
    }
}

void alp_free_pool(POOL_REF_T id){
    alp_free(id);
    pool_t *pool = map_get(&pools, id);
    da_free(*pool);
    map_remove(&pools, id);
    log_trace("Deleting allocation pool: %s", id);
}