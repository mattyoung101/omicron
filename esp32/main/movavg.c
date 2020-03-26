#include "movavg.h"
#include <string.h>
#include <stdio.h>

// Simple moving average in C, v1.1
// Copyright (c) 2019-2020 Matt Young. Licensed into the public domain with the Unlicense.

movavg_t *movavg_create(size_t size){
    movavg_t *movavg = calloc(1, sizeof(movavg_t));
    movavg->maxSize = size;
    movavg->items = calloc(size, sizeof(double));
    movavg->counter = 0;
    return movavg;
}

void movavg_free(movavg_t *movavg){
    free(movavg->items);
    free(movavg);
}

void movavg_push(movavg_t *movavg, double value){
    // believe it or not, these brackets are actually required due to operator precedence
    movavg->counter = (movavg->counter + 1) % (movavg->maxSize);
    movavg->items[movavg->counter] = value;
}

double movavg_calc(movavg_t *movavg){
    double sum = 0;
    for (size_t i = 0; i < movavg->counter; i++){
        sum += movavg->items[i];
    }
    // hack so we don't divide by zero, TODO find a better solution than this
    if (movavg->counter == 0) return movavg->items[0];
    return sum / (double) movavg->counter;
}

void movavg_clear(movavg_t *movavg){
    memset(movavg->items, 0, movavg->maxSize * sizeof(double));
    movavg->counter = 0;
}

void movavg_resize(movavg_t *movavg, size_t newSize){
    movavg->items = realloc(movavg->items, newSize);
    movavg->counter = 0;
    movavg->maxSize = newSize;
}

void movavg_dump(movavg_t *movavg){
    double value = movavg_calc(movavg);
    printf("Moving average object: %zu items, counter at %zu, value is: %f\n", movavg->maxSize, movavg->counter, value);
    for (size_t i = 0; i < movavg->maxSize; i++){
        printf("    %zu. %f\n", i, movavg->items[i]);
    }
}