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
#include "rpa_queue.h"

float estimatedX = 0.0f;
float estimatedY = 0.0f;
static nlopt_opt optimiser;
static pthread_t workThread;
// we could probably just handle this with a single pthread condition and mutex, but just in case the localiser is too slow
// I want it to be able to support a backlog as well (otherwise we'd have to block the main thread until the last frame
// finishes optimising)
static rpa_queue_t *queue;

/**
 * The bread and butter of our localisation system, this is the function to be minimised by the subplex optimiser.
 * What this does is basically given an x,y coordinate pair returns a "score" of how close the estimated position of
 * the robot is to the real one.
 * @param n the number of dimensions, should be always 2
 * @param x an N dimensional array containing the input values
 * @param grad the gradient, ignored as we're using a derivative-free optimisation algorithm
 * @param f_data ignored
 * @return a score of how close the estimated position is to the real position
 */
static double objective_function(unsigned n, const double* x, double* grad, void* f_data){
    // this will likely be ethan's implementation of the objective function where we look up the value in the field map

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

/** The worker thread for the localiser, basically just calls nlopt_optimize() and transmits the result */
static void *work_thread(void *arg){
    log_trace("Localiser work thread started");
    while (true){
        void *queueData = NULL;
        if (!rpa_queue_pop(queue, &queueData)){
            log_warn("Failed to pop item from localisation queue");
            continue;
        }

        localiser_entry_t *entry = (localiser_entry_t*) queueData;
        // so what we'll do here is submit the frame to the GPU/CUDA program only once to save re-uploading it on each
        // objective function evaluation
        free(entry);
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
    log_info("Field information: grid size is %d cm, contains %d cells, dimensions are %dx%d cm",
            field.unitDistance, field.cellCount, field.fieldWidth, field.fieldHeight);

    // create a two dimensional subplex optimiser. subplex is an algorithm similar to Nelder-Mead simplex but it's
    // faster and more stable.
    optimiser = nlopt_create(NLOPT_LN_SBPLX, 2);
    nlopt_set_stopval(optimiser, LOCALISER_ERROR_TOLERANCE); // stop if we're close enough to a solution
    nlopt_set_ftol_abs(optimiser, LOCALISER_STEP_TOLERANCE); // stop if the last step was too small (we must be going nowhere/solved)
    nlopt_set_maxtime(optimiser, LOCALISER_MAX_EVAL_TIME); // stop if it's taking too long
    nlopt_set_min_objective(optimiser, objective_function, NULL);
    // TODO set lower and upper bounds to field dimensions noting that (0, 0) is the centre

    double minCoord[] = {-field.fieldWidth / 2.0, -field.fieldHeight / 2.0};
    double maxCoord[] = {field.fieldWidth / 2.0, field.fieldHeight / 2.0};
    log_trace("Min bound: (%f,%f) Max bound: (%f,%f)", minCoord[0], minCoord[1], maxCoord[0], maxCoord[1]);
    nlopt_set_lower_bounds(optimiser, minCoord);
    nlopt_set_upper_bounds(optimiser, maxCoord);

    // create work thread
    rpa_queue_create(&queue, 4);
    int err = pthread_create(&workThread, NULL, work_thread, NULL);
    if (err != 0){
        log_error("Failed to create localiser thread: %s", strerror(err));
    } else {
        pthread_setname_np(workThread, "Localiser Thrd");
    }

    free(data);
}

void localiser_post(uint8_t *frame, uint16_t width, uint16_t height){
    // the input we receive will be the thresholded mask of only the lines (1 bit greyscale), so what we do is:
    // 1. cast out lines using Bresenham's line algorithm in OpenCL to get our localisation points
    // 2. optimise to find new estimated position using NLopt's Subplex algorithm (better than NM-Simplex) - cannot be threaded
    // 3. publish result to esp32 over UART

    localiser_entry_t *entry = malloc(sizeof(localiser_entry_t));
    entry->frame = frame;
    entry->width = width;
    entry->height = height;
    if (!rpa_queue_trypush(queue, entry)){
        log_warn("Failed to push new localisation entry to queue. This likely indicates a performance or hang in the optimiser.");
        free(frame);
        free(entry);
    }
}

void localiser_dispose(void){
    log_trace("Disposing localiser");
    nlopt_destroy(optimiser);
    pthread_cancel(workThread);
    rpa_queue_destroy(queue);
}