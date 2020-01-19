#include "movavg.h"
#include <string.h>
#include <stdio.h>

// Moving average, by Matt Young
// Licensed into the public domain with the Unlicense.

movavg_t *movavg_create(size_t size){
    movavg_t *movavg = calloc(1, sizeof(movavg_t));
    movavg->size = size;
    movavg->items = calloc(size, sizeof(double));
    movavg->counter = 0;
    return movavg;
}

void movavg_free(movavg_t *mov_avg){
    free(mov_avg);
    mov_avg = NULL;
}

void movavg_push(movavg_t *mov_avg, double value){
    mov_avg->items[mov_avg->counter++ % mov_avg->size] = value;
}

double movavg_calc(movavg_t *mov_avg){
    double sum = 0;
    for (int i = 0; i < mov_avg->counter; i++){
        sum += mov_avg->items[i];
    }
    return sum / (double) mov_avg->counter;
}

void movavg_clear(movavg_t *mov_avg){
    memset(mov_avg->items, 0, mov_avg->size * sizeof(double));
    mov_avg->counter = 0;
}