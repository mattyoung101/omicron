#include "localisation.h"
#include <log/log.h>
#include <nlopt.h>
#include <stdbool.h>
#include <nanopb/pb_decode.h>
#include "defines.h"
#include "utils.h"
#include "protobuf/FieldFile.pb.h"
#include <pthread.h>
#include <unistd.h>

float estimatedX = 0.0f;
float estimatedY = 0.0f;
static nlopt_opt optimiser;
static pthread_t workThread;

static double objective_function(unsigned n, const double* x, double* grad, void* f_data){
    // this will likely be ethan's implementation of the objective function where we look up the value in the
    // field map

    /** ALGORITHM OUTLINE
     * Get the relative coordinates of the points at which the rays intersect the white line (let's call these line points)
     * Rotate all line points around the centre of the robot based on the current heading
     * Translate all line points to the current estimated position of the robot (wherever the algorithm is testing)
     * Round the coordinates of each line point to the nearest cm
     * Lookup the smallest distance for each line point to the closest line in the massive field file array
     * Sum all the distances together
     * 
     * A potential micro-optimisation here is to arrange the algorithm as such:
     * 
     * FOR EACH LINE POINT
     *     Rotate
     *     Translate
     *     Round
     *     Lookup
     *     Add to current working total
     * 
     * This gives us a value for which the optimiser will aim to minimise. The closer the value is to 0, the better the line points align with the white line
    **/ 

    return 0.0;
}

static void *work_thread(void *arg){
    log_trace("Localiser work thread started");
    while (true){
        sleep(0xF0F0FEFE);
    }
    return NULL;
}

void localiser_init(char *fieldFile){
    log_info("Initialising localiser with field file: %s", fieldFile);
    long fileSize = 0;
    uint8_t *data = utils_load_bin(fieldFile, &fileSize);
    log_trace("Field file is %ld KiB", fileSize / 1024);

    pb_istream_t stream = pb_istream_from_buffer(data, fileSize);
    FieldFile field = FieldFile_init_zero;
    if (!pb_decode(&stream, FieldFile_fields, &field)){
        log_error("Failed to decode field file: %s", PB_GET_ERROR(&stream));
    }
    free(data);
    log_info("Field information: grid size is %d cm, contains %d cells, dimensions are %dx%d cm",
            field.unitDistance, field.cellCount, field.fieldWidth, field.fieldHeight);

    // create a two dimensional subplex optimiser. subplex is an algorithm similar to Nelder-Mead simplex but it's
    // faster and more stable.
    optimiser = nlopt_create(NLOPT_LN_SBPLX, 2);
    nlopt_set_stopval(optimiser, LOCALISER_ERROR_TOLERANCE); // stop if we're close enough to a solution
    nlopt_set_ftol_abs(optimiser, LOCALISER_STEP_TOLERANCE); // stop if the last step was too small (we must be going nowhere/solved)
    nlopt_set_maxtime(optimiser, LOCALISER_MAX_EVAL_TIME); // stop if it's taking too long

    // create work thread
    int err = pthread_create(&workThread, NULL, work_thread, NULL);
    if (err != 0){
        log_error("Failed to create localiser thread: %s", strerror(err));
    } else {
        pthread_setname_np(workThread, "Localiser Thrd");
    }
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
    pthread_cancel(workThread);
}