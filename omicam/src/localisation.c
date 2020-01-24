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
#include <math.h>
#include "DG_dynarr.h"
#include "mathc.h"
#include "stb_image_write.h"
#include <sys/param.h>

DA_TYPEDEF(struct vec2, vec2_array_t);

static nlopt_opt optimiser;
static pthread_t workThread;
static FieldFile field = FieldFile_init_zero;
// we could probably just handle this with a single pthread condition and mutex, but just in case the localiser is too slow
// I want it to be able to support a backlog as well (otherwise we'd have to block the main thread until the last frame
// finishes optimising)
static rpa_queue_t *queue;
static vec2_array_t linePoints = {0};
static vec2_array_t correctedLinePoints = {0};
//static uint8_t *outImage = NULL;
_Atomic bool localiserDone = false;
static double orientation = 0.0; // last received orientation from ESP32

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

/**
 * Uses Bresenham's line algorithm to draw a line from (x0, y0) to (x1, y1), adding a line point to the linked list at
 * the start and end of each white object.
 */
static void raycast(const uint8_t *image, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t imageWidth){
    bool wasLine = false; // true if the last pixel we accessed was on the line

    // source: https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
    int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int32_t dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int32_t err = (dx > dy ? dx : -dy) / 2;
    int32_t e2;

    while (true){
        int32_t i = x0 + imageWidth * y0;
        bool isLine = image[i] != 0;

        if (isLine && !wasLine){
            // we just entered the line so add a point
            struct vec2 pos = {(double) x0, (double) y0};
            da_push(linePoints, pos);
            wasLine = true;
        } else if (!isLine && wasLine){
            // we just exited the line so add a point
            struct vec2 pos = {(double) x0, (double) y0};
            da_push(linePoints, pos);
            wasLine = false;
        }

        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx){
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

/** The worker thread for the localiser, basically just calls nlopt_optimize() and transmits the result */
static void *work_thread(void *arg){
    log_trace("Localiser work thread started");
    while (true){
        pthread_testcancel();
        void *queueData = NULL;
        if (!rpa_queue_pop(queue, &queueData)){
            log_warn("Failed to pop item from localisation queue");
            continue;
        }

        localiser_entry_t *entry = (localiser_entry_t*) queueData;
        da_clear(linePoints);
        da_clear(correctedLinePoints);
        localiserDone = false;

        // 1. calculate begin points for Bresenham's line algorithm
        double interval = PI2 / LOCALISER_NUM_RAYS;
        double x0 = entry->width / 2.0;
        double y0 = entry->height / 2.0;

        // 1.1. cast line segments from the centre of the mirror to the edge of it, to calculate the line points (NOLINTNEXTLINE)
        for (double angle = 0.0; angle < PI2; angle += interval){
            double r = (double) visionMirrorRadius;
            double x1 = x0 + (r * cos(angle));
            double y1 = y0 + (r * sin(angle));
            raycast(entry->frame, ROUND2INT(x0), ROUND2INT(y0), ROUND2INT(x1), ROUND2INT(y1), entry->width);
        }

        // don't bother localising if no lines are present
        if (da_count(linePoints) == 0){
            goto cleanup;
        }

        // 2. dewarp points based on calculated mirror model, and rotate based on robot's real orientation
        for (size_t i = 0; i < da_count(linePoints); i++){
            struct vec2 point = da_get(linePoints, i);
            struct vec2 dewarped = {0};
            dewarped.x = utils_camera_dewarp(point.x);
            dewarped.y = utils_camera_dewarp(point.y);
            svec2_rotate(dewarped, orientation);
            da_add(correctedLinePoints, dewarped);
        }

        cleanup:
        free(entry->frame);
        free(entry);
        localiserDone = true;
        pthread_testcancel();
    }
    return NULL;
}

void localiser_init(char *fieldFile){
    log_info("Initialising localiser with field file: %s", fieldFile);
    long fileSize = 0;
    uint8_t *data = utils_load_bin(fieldFile, &fileSize);
    log_trace("Field file is %ld KiB", fileSize / 1024);

    pb_istream_t stream = pb_istream_from_buffer(data, fileSize);
    if (!pb_decode(&stream, FieldFile_fields, &field)){
        log_error("Failed to decode field file: %s", PB_GET_ERROR(&stream));
    }
    log_info("Field information: grid size is %d cm, contains %d cells, dimensions are %dx%d cm",
            field.unitDistance, field.cellCount, field.length, field.width);
    log_info("Using %d rays (ray interval is %.2f radians or %.2f degrees)", LOCALISER_NUM_RAYS, PI2 / LOCALISER_NUM_RAYS,
            360.0 / LOCALISER_NUM_RAYS);

    // create a two dimensional Subplex optimiser
    optimiser = nlopt_create(NLOPT_LN_SBPLX, 2);
    nlopt_set_stopval(optimiser, LOCALISER_ERROR_TOLERANCE); // stop if we're close enough to a solution
    nlopt_set_ftol_abs(optimiser, LOCALISER_STEP_TOLERANCE); // stop if the last step was too small (we must be going nowhere/solved)
    nlopt_set_maxtime(optimiser, LOCALISER_MAX_EVAL_TIME); // stop if it's taking too long
    nlopt_set_min_objective(optimiser, objective_function, NULL);

    double minCoord[] = {-field.length / 2.0, -field.width / 2.0};
    double maxCoord[] = {field.length / 2.0, field.width / 2.0};
    log_trace("Min bound: (%.2f,%.2f) Max bound: (%.2f,%.2f)", minCoord[0], minCoord[1], maxCoord[0], maxCoord[1]);
    nlopt_set_lower_bounds(optimiser, minCoord);
    nlopt_set_upper_bounds(optimiser, maxCoord);

    // create work thread
    rpa_queue_create(&queue, 2);
    int err = pthread_create(&workThread, NULL, work_thread, NULL);
    if (err != 0){
        log_error("Failed to create localiser thread: %s", strerror(err));
    } else {
        pthread_setname_np(workThread, "Localiser Thrd");
    }

    free(data);
    pthread_testcancel();
}

void localiser_post(uint8_t *frame, int32_t width, int32_t height){
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

uint32_t localiser_remote_get_points(RDPoint *array, size_t arraySize){
    if (da_count(linePoints) > arraySize){
        //log_warn("Line points size overflow. Please increase the max_count for linePoints in RemoteDebug.options.");
    }

    // this is to stop it from overflowing the Protobuf fixed size buffer
    int32_t size = MIN(arraySize, da_count(linePoints));

    for (int i = 0; i < size; i++){
        struct vec2 point = da_get(linePoints, i);
        array[i].x = ROUND2INT(point.x);
        array[i].y = ROUND2INT(point.y);
    }

    return size;
}

void localiser_dispose(void){
    log_trace("Disposing localiser");
    pthread_cancel(workThread);
    pthread_join(workThread, NULL);
    nlopt_destroy(optimiser);
    rpa_queue_destroy(queue);
    da_free(linePoints);
    da_free(correctedLinePoints);
}