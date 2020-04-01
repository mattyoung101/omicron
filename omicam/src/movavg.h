#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Simple moving average in C, v1.2
// Copyright (c) 2019-2020 Matt Young. Licensed into the public domain with the Unlicense.

typedef struct {
    /** max size the user can store in the buffer */
    size_t maxSize;
    /** current position in the buffer */
    size_t counter;
    /** array of values, size of this array is always maxSize */
    double *items;
    /** array the same size of the items array, each element is true if it's been written at some point */
    bool *writtenValues;
} movavg_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Instantiates a new moving average object
 * @param size the max size of the buffer
 */
movavg_t *movavg_create(size_t size);
/** Frees all resources from a movavg object. After this, the pointer is no longer valid. */
void movavg_free(movavg_t *movavg);
/** Pushes the value to the moving average ringbuffer. */
void movavg_push(movavg_t *movavg, double value);
/** Calculates the moving average of the buffer. */
double movavg_calc(movavg_t *movavg);
/** Resets the moving average buffer to the beginning. Does not free any memory. */
void movavg_clear(movavg_t *movavg);
/** Grows/shrinks the moving average buffer to the new size specified. */
void movavg_resize(movavg_t *movavg, size_t newSize);
/** Prints the backing array to stdout */
void movavg_dump(movavg_t *movavg);

#ifdef __cplusplus
};
#endif