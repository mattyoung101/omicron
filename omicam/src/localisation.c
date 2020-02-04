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
static FieldFile field = {0};
// we could probably just handle this with a single pthread condition and mutex, but just in case the localiser is too slow
// I want it to be able to support a backlog as well (otherwise we'd have to block the main thread until the last frame
// finishes optimising)
static rpa_queue_t *queue;
static vec2_array_t linePoints = {0};
static vec2_array_t correctedLinePoints = {0};
//static uint8_t *outImage = NULL;
_Atomic bool localiserDone = false;
static double orientation = 0.0; // last received orientation from ESP32

static inline int32_t constrain(int32_t x, int32_t min, int32_t max){
    if (x < min){
        return min;
    } else if (x > max){
        return max;
    } else {
        return x;
    }
}

/** Renders the objective function for the whole field and then quits the application **/
static void render_test_image(){
    uint8_t *image = malloc(field.width * field.length);
    double *data = malloc(field.width * field.length * sizeof(double));

    // calculate objective function for all data points
    for (int32_t y = 0; y < field.width; y++){
        for (int32_t x = 0; x < field.length; x++){
            double totalError = 0.0;
            for (size_t i = 0; i < da_count(correctedLinePoints); i++){
                struct vec2 point = da_get(correctedLinePoints, i);

                // translate coordinate to localiser guessed position
                point.x += x;
                point.y += y;

                // convert to field file coordinates and sum
                int32_t fx = constrain(ROUND2INT(point.x + (field.length / 2.0)), 0, field.length);
                int32_t fy = constrain(ROUND2INT(point.y + (field.width / 2.0)), 0, field.width);
                int32_t index = (int32_t) (fy + (field.width / field.unitDistance) * fx);
                totalError += field.data[index];
            }

            data[x + field.length * y] = totalError;
        }
    }

    // find min and max of array
    double currentMin = INT32_MAX; // worst possible
    double currentMax = INT32_MIN; // worst possible
    for (int32_t i = 0; i < (field.width * field.length); i++){
        if (data[i] < currentMin){
            currentMin = data[i];
        } else if (data[i] > currentMax){
            currentMax = data[i];
        }
    }
    log_trace("currentMin: %.2f, currentMax: %.2f", currentMin, currentMax);

    // scale each value
//    for (int32_t i = 0; i < (field.width * field.length); i++){
//        double val = data[i];
//        double scaled = (val - currentMin) / (currentMax - currentMin);
//        image[i] = (uint8_t) (scaled * 255);
//    }
    for (int32_t y = 0; y < field.width; y++){
        for (int32_t x = 0; x < field.length; x++) {
            int32_t index = x + field.length * y;
            double val = data[index];
            double scaled = (val - currentMin) / (currentMax - currentMin);
            image[index] = (uint8_t) (scaled * 255);
        }
    }

    // render image
    stbi_write_bmp("../objective_function.bmp", field.length, field.width, 1, image);

    exit(EXIT_SUCCESS);
}

/**
 * This function will be minimised by the Subplex optimiser. Based on pseudocode, research and field files by Ethan.
 * Poorly documented because it's complicated :(
 * @param n the number of dimensions, should be always 2
 * @param x an N dimensional array containing the input values
 * @param grad the gradient, ignored as we're using a derivative-free optimisation algorithm
 * @param f_data user data, ignored
 * @return a score of how close the estimated position is to the real position
 */
static double objective_function(unsigned n, const double* x, double* grad, void* f_data){
    double totalError = 0.0;
    for (size_t i = 0; i < da_count(correctedLinePoints); i++){
        struct vec2 point = da_get(correctedLinePoints, i);

        // translate coordinate to localiser guessed position
        point.x += x[0];
        point.y += x[1];

        // convert to field file coordinates and sum
        int32_t fx = constrain(ROUND2INT(point.x + (field.length / 2.0)), 0, field.length);
        int32_t fy = constrain(ROUND2INT(point.y + (field.width / 2.0)), 0, field.width);
        int32_t index = (int32_t) (fx + (field.width / field.unitDistance) * fy);
        totalError += field.data[index];
    }

    return totalError;
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
            log_warn("Failed to pop item from localisation work queue");
            continue;
        }

        localiser_entry_t *entry = (localiser_entry_t*) queueData;
        da_clear(linePoints);
        da_clear(correctedLinePoints);
        localiserDone = false;

        // image analysis
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

        // image normalisation
        // 2. dewarp points based on calculated mirror model to get cm field coord, then rotate based on robot's real orientation
        for (size_t i = 0; i < da_count(linePoints); i++){
            struct vec2 point = da_get(linePoints, i);
            point.x -= entry->width / 2.0;
            point.y -= entry->height / 2.0;

            // 2.1. convert the point to polar coordinates to apply dewarp
            double r = utils_camera_dewarp(svec2_length(point));
            double theta = svec2_angle(point);

            // 2.2. convert back to cartesian and rotate
            struct vec2 dewarped = {r * cos(theta), r * sin(theta)};
            svec2_rotate(dewarped, orientation);
            da_add(correctedLinePoints, dewarped);
        }

        // coordinate optimisation
        // 3. start the NLopt Subplex optimiser
        double resultCoord[2] = {0.0, 0.0}; // TODO use last mouse sensor coordinate to seed optimisation
        double resultError = 0.0;
        nlopt_result result = nlopt_optimize(optimiser, resultCoord, &resultError);
        if (result < 0){
            // seem as though we've been stitched up and the NLopt version Ubuntu ships doesn't support nlopt_result_to_string()
            log_warn("The optimiser may have failed to converge on a solution: code %d", result);
        }
        printf("optimiser done with coordinate: %.2f,%.2f, error: %.2f, result id: %s\n", resultCoord[0], resultCoord[1],
               resultError, nlopt_result_to_string(result));
        localisedPosition.x = resultCoord[0];
        localisedPosition.y = resultCoord[1];

        render_test_image();

        // TODO post results with uart and also send it to the performance evaluator (like the FPS logger but for optimiser)

        cleanup:
        free(entry->frame);
        free(entry);
        localiserDone = true;
        pthread_testcancel();
    }
    return NULL;
}

void localiser_init(char *fieldFile){
    int major, minor, bugfix;
    nlopt_version(&major, &minor, &bugfix);
    log_info("NLopt version: %d.%d.%d", major, minor, bugfix);

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
    nlopt_set_maxtime(optimiser, LOCALISER_MAX_EVAL_TIME / 1000.0); // stop if it's taking too long
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
        // log_warn("Failed to push new localisation entry to queue. This likely indicates a performance or hang in the optimiser.");
        free(frame);
        free(entry);
    }
}

uint32_t localiser_remote_get_points(RDPoint *array, size_t arraySize, bool dewarped){
    vec2_array_t arr = dewarped ? correctedLinePoints : linePoints;

    if (da_count(arr) > arraySize){
        //log_warn("Line points size overflow. Please increase the max_count for linePoints in RemoteDebug.options.");
    }

    // this is to stop it from overflowing the Protobuf fixed size buffer
    int32_t size = MIN(arraySize, da_count(arr));

    for (int i = 0; i < size; i++){
        struct vec2 point = da_get(arr, i);
        array[i].x = ROUND2INT(point.x);
        array[i].y = ROUND2INT(point.y);
    }

    return size;
}

void localiser_dispose(void){
    log_trace("Disposing localiser");
    pthread_cancel(workThread);
    pthread_join(workThread, NULL);
    nlopt_force_stop(optimiser);
    nlopt_destroy(optimiser);
    rpa_queue_destroy(queue);
    da_free(linePoints);
    da_free(correctedLinePoints);
}