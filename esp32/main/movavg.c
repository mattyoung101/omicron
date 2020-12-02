#include "movavg.h"
#include <string.h>
#include <stdio.h>

// Simple moving average in C, v1.2
// Copyright (c) 2019-2020 Matt Young. Licensed into the public domain with the Unlicense.

movavg_t *movavg_create(size_t size){
    movavg_t *movavg = calloc(1, sizeof(movavg_t));
    movavg->maxSize = size;
    movavg->items = calloc(size, sizeof(double));
    movavg->writtenValues = calloc(size, sizeof(bool));
    movavg->counter = 0;
    return movavg;
}

void movavg_free(movavg_t *movavg){
    free(movavg->items);
    free(movavg->writtenValues);
    free(movavg);
}

void movavg_push(movavg_t *movavg, double value){
    // believe it or not, these brackets are actually required due to operator precedence
    movavg->counter = (movavg->counter + 1) % (movavg->maxSize);
    movavg->items[movavg->counter] = value;
    movavg->writtenValues[movavg->counter] = true;
}

double movavg_calc(movavg_t *movavg){
    double sum = 0;
    // number of points we actually considered in the sum
    double visitedValues = 0;

    for (size_t i = 0; i < movavg->maxSize; i++){
        if (movavg->writtenValues[i]) {
            sum += movavg->items[i];
            visitedValues++;
        }
    }

    return sum / visitedValues;
}

void movavg_clear(movavg_t *movavg){
    memset(movavg->items, 0, movavg->maxSize * sizeof(double));
    memset(movavg->writtenValues, 0, movavg->maxSize * sizeof(bool));
    movavg->counter = 0;
}

void movavg_resize(movavg_t *movavg, size_t newSize){
    // TODO unsure if items and writtenValues will line up again after resize?
    movavg->items = realloc(movavg->items, newSize);
    movavg->writtenValues = realloc(movavg->writtenValues, newSize);
    movavg->counter = 0;
    movavg->maxSize = newSize;
}

void movavg_dump(movavg_t *movavg){
    double value = movavg_calc(movavg);
    printf("Moving average object: %zu items, counter at %zu, value is: %f\n", movavg->maxSize, movavg->counter, value);
    for (size_t i = 0; i < movavg->maxSize; i++){
        printf("    %zu. %f (written: %s)\n", i, movavg->items[i], movavg->writtenValues[i] ? "true" : "false");
    }
}
