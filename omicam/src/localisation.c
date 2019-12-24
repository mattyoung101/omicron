#include <log/log.h>
#include "localisation.h"

float estimatedX = 0.0f;
float estimatedY = 0.0f;

void localiser_init(char *fieldFile){
    log_info("Initialising localiser with field file: %s", fieldFile);
    // load file, parse protobuf, init CL program, creat NLopt instance
}

void localiser_post(uint8_t *frame, uint16_t width, uint16_t height){
    // the input we receive will be the thresholded mask of only the lines (1 bit greyscale), so what we do is:
    // 1. cast out lines using Bresenham's line algorithm in OpenCL to get our localisation points
    // 2. optimise to find new estimated position using NLopt's Subplex algorithm (better than NM-Simplex) - cannot be threaded
    // 3. publish result to esp32 over UART
}

void localiser_dispose(void){
    // delete CL, destroy NLopt, free any other resources
    log_trace("Disposing localiser");
}