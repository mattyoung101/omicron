#include <log/log.h>
#include <nlopt.h>
#include <stdbool.h>
#include "localisation.h"
#include "defines.h"

float estimatedX = 0.0f;
float estimatedY = 0.0f;
static nlopt_opt optimiser;

static double objective_function(unsigned n, const double* x, double* grad, void* f_data){
    // this will likely be ethan's implementation of the objective function where we look up the value in the
    // field map
    return 0.0;
}

static void *localisation_work_thread(void *arg){
    while (true){

    }
    return NULL;
}

void localiser_init(char *fieldFile){
    log_info("Initialising localiser with field file: %s", fieldFile);
    // load file, parse protobuf, init cuda program, creat NLopt instance

    // create a two dimensional subplex optimiser. subplex is an algorithm similar to Nelder-Mead simplex but it's
    // faster and more stable.
    optimiser = nlopt_create(NLOPT_LN_SBPLX, 2);
    nlopt_set_stopval(optimiser, LOCALISER_ERROR_TOLERANCE); // stop if we're close enough to a solution
    nlopt_set_ftol_abs(optimiser, LOCALISER_STEP_TOLERANCE); // stop if the last step was too small (we must be going nowhere/solved)
    nlopt_set_maxtime(optimiser, LOCALISER_MAX_EVAL_TIME); // stop if it's taking too long
}

void localiser_post(uint8_t *frame, uint16_t width, uint16_t height){
    // the input we receive will be the thresholded mask of only the lines (1 bit greyscale), so what we do is:
    // 1. cast out lines using Bresenham's line algorithm in OpenCL to get our localisation points
    // 2. optimise to find new estimated position using NLopt's Subplex algorithm (better than NM-Simplex) - cannot be threaded
    // 3. publish result to esp32 over UART
}

void localiser_dispose(void){
    // delete cuda, destroy NLopt, free any other resources
    log_trace("Disposing localiser");
    nlopt_destroy(optimiser);
}