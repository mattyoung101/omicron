#pragma once
#include <stdint.h>
#include <stdlib.h>

// Moving average, by Matt Young
// Licensed into the public domain with the Unlicense.

typedef struct {
    size_t size;
    int32_t counter;
    double *items;
} movavg_t;

#ifdef __cplusplus
extern "C" {
#endif

/** Instantiates a new moving average object **/
movavg_t *movavg_create(size_t size);

/** Frees a moving average object **/
void movavg_free(movavg_t *mov_avg);

/** Pushes a new value to the moving average items array, looping around to the start if required **/
void movavg_push(movavg_t *mov_avg, double value);

/** Calculates the moving average **/
double movavg_calc(movavg_t *mov_avg);

void movavg_clear(movavg_t *mov_avg);

#ifdef __cplusplus
};
#endif