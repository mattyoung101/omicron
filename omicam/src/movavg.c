#include "movavg.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Simple moving average in C, v1.3
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

double movavg_calc_median(movavg_t *movavg){
    // find number of written values that we can use
    size_t i = 0;
    for (i = 0; i < movavg->maxSize; i++){
        if (!movavg->writtenValues[i]) break;
    }

    double medianIndex = ((double) i + 1) / 2.0;
    if (round(medianIndex) == medianIndex){
        // whole number so we return the value directly
        return movavg->items[(size_t) medianIndex];
    } else {
        // otherwise if it's a decimal average the above and below indices
        size_t aboveIndex = ceil(medianIndex);
        size_t belowIndex = floor(medianIndex);
        return (movavg->items[aboveIndex] + movavg->items[belowIndex]) / 2.0;
    }
}

void movavg_clear(movavg_t *movavg){
    memset(movavg->items, 0, movavg->maxSize * sizeof(double));
    memset(movavg->writtenValues, 0, movavg->maxSize * sizeof(bool));
    movavg->counter = 0;
}

void movavg_resize(movavg_t *movavg, size_t newSize){
    movavg->items = realloc(movavg->items, newSize);
    movavg->writtenValues = realloc(movavg->writtenValues, newSize);
    movavg->maxSize = newSize;

    // we must reset here to ensure defined behaviour (if we grow, the newly allocated contents is undefined)
    movavg_clear(movavg);
}

void movavg_dump(movavg_t *movavg){
    double value = movavg_calc(movavg);
    printf("Moving average object: %zu items, counter at %zu, value is: %f\n", movavg->maxSize, movavg->counter, value);
    for (size_t i = 0; i < movavg->maxSize; i++){
        printf("    %zu. %f (written: %s)\n", i, movavg->items[i], movavg->writtenValues[i] ? "true" : "false");
    }
}