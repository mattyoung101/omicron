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
#include "map.h"

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
static vec2_array_t objectivePoints = {0};
//static uint8_t *outImage = NULL;
_Atomic bool localiserDone = false;
static double orientation = 0.0; // last received orientation from ESP32
static uint8_t *outImage = NULL;
static uint32_t totalPoints = 0;
static map_double_t minDistMap = {0};

static inline int32_t constrain(int32_t x, int32_t min, int32_t max){
    if (x < min){
        return min;
    } else if (x > max){
        return max;
    } else {
        return x;
    }
}

/**
* Uses Bresenham's line algorithm to draw a line from (x0, y0) to (x1, y1), adding a line point to the linked list at
* the start and end of each white object.
*/
static void raycast(const uint8_t *image, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t imageWidth, struct vec2 minCorner,
        struct vec2 maxCorner, vec2_array_t *array){
    bool wasLine = false; // true if the last pixel we accessed was on the line

    // source: https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
    int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int32_t dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int32_t err = (dx > dy ? dx : -dy) / 2;
    int32_t e2;

    while (true){
        if (x0 < minCorner.x || y0 < minCorner.y || x0 > maxCorner.x || y0 > maxCorner.y){
            //log_warn("Raycasting out of bounds at %d,%d", x0, y0);
            return;
        }

        int32_t i = x0 + imageWidth * y0;
        bool isLine = image[i] != 0;

        if (isLine && !wasLine){
            // we just entered the line so add a point
            struct vec2 pos = {(double) x0, (double) y0};
            da_push(*array, pos);
            wasLine = true;
        } else if (!isLine && wasLine){
            // we just exited the line so add a point
            struct vec2 pos = {(double) x0, (double) y0};
            da_push(*array, pos);
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

static int minimum_dist_cmp(const void *a, const void *b){
    struct vec2 avec = *(struct vec2*) a;
    struct vec2 bvec = *(struct vec2*) b;

    // lookup avec and bvec in the hashmap
    char akey[64];
    char bkey[64];
    sprintf(akey, "%.2f,%.2f", avec.x, avec.y);
    sprintf(bkey, "%.2f,%.2f", bvec.x, bvec.y);

    double *adist = map_get(&minDistMap, akey);
    double *bdist = map_get(&minDistMap, bkey);

    if (adist == NULL || bdist == NULL){
        log_warn("Either %s or %s not found in map!", akey, bkey);
        return 0;
    }

    return (adist > bdist) - (adist < bdist);
}

/**
 * This function will be minimised by the Subplex optimiser. Based on research by Ethan and inefficient algorithm impl
 * by Matt.
 * @param x the guessed x position in field coordinates
 * @param y the guessed y position in field coordinates
 * @return a score of how close the estimated position is to the real position
 */
static inline double objective_func_impl(double x, double y){
    da_clear(objectivePoints);
    map_init(&minDistMap);
    double begin = utils_time_millis();

    // 1. raycast out from the guessed x and y on the field file
    double interval = PI2 / LOCALISER_NUM_RAYS;
    double x0 = x;
    double y0 = y;
    double maxDist = sqrt(field.width * field.width + field.length * field.length);

    // note that these are in field file coordinates, not localiser coordinates (the ones with 0,0 as the field centre)
    struct vec2 minCoord = {0, 0};
    struct vec2 maxCoord = {field.length, field.width};

    // 1.2. loop over the points and raycast (NOLINTNEXTLINE)
    for (double angle = 0.0; angle < PI2; angle += interval){
        double r = maxDist;
        double x1 = x0 + (r * cos(angle));
        double y1 = y0 + (r * sin(angle));
        raycast(field.data.bytes, X_TO_FF(x0), Y_TO_FF(y0), X_TO_FF(x1), Y_TO_FF(y1), field.length, minCoord, maxCoord, &objectivePoints);
    }

    // FIXME should have a check to see if there's no points return a really large value

    // 2. similarity calculation: the actual bread and butter of the objective function
    // now that we've determined what it should look like if the robot was in the guessed position, we need to compare
    // it to what we actually observed - which we calculated earlier outside the objective function

    // set B will always be larger than set A
    vec2_array_t setA = da_count(objectivePoints) > da_count(correctedLinePoints) ? correctedLinePoints : objectivePoints;
    vec2_array_t setB = da_count(objectivePoints) >= da_count(correctedLinePoints) ? objectivePoints : correctedLinePoints;

    // 2.1. for each point in set B, calculate the minimum distance to any point in set A and store it in a lookup table
    for (size_t i = 0; i < da_count(setB); i++){
        struct vec2 bpoint = da_get(setB, i);

        // loop through and find the smallest distance from this point to the one in set A
        double minDist = HUGE_VAL;
        for (size_t j = 0; j < da_count(setA); j++){
            struct vec2 apoint = da_get(setA, j);
            double dist = svec2_distance(apoint, bpoint);
            if (dist < minDist){
                minDist = dist;
            }
        }

        // add to hashmap
        char key[64];
        sprintf(key, "%.2f,%.2f", bpoint.x, bpoint.y);
        map_set(&minDistMap, key, minDist);
    }

    // 2.2 sort set B based on LUT
    da_sort(setB, minimum_dist_cmp);

    // 3. pick the top N and sum them and return the sum
    double totalError = 0.0f;
    for (size_t i = 0; i < da_count(setA); i++){
        struct vec2 point = da_get(setB, i);
        char key[64];
        sprintf(key, "%.2f,%.2f", point.x, point.y);

        double *dist = map_get(&minDistMap, key);
        if (dist == NULL){
            log_warn("Key %s not found in map", key);
            continue;
        }
        totalError += *dist;
    }

    // printf("took: %2.f ms with %zu points\n", utils_time_millis() - begin, da_count(objectivePoints));
    map_deinit(&minDistMap);
    totalPoints += da_count(objectivePoints);
    //return (double) da_count(objectivePoints);
    return totalError;
}

/** Renders the objective function for the whole field and then quits the application **/
static void render_test_image(){
    outImage = calloc(field.width * field.length, sizeof(uint8_t));
    double *data = calloc(field.width * field.length, sizeof(double));

    // calculate objective function for all data points
    for (int32_t y = 0; y < field.width; y++){
        for (int32_t x = 0; x < field.length; x++){
            data[x + field.length * y] = objective_func_impl((double) x, (double) y);
        }
    }

    // find min and max of array, starting with worst possible values
    double currentMin = HUGE_VAL;
    double currentMax = -HUGE_VAL;
    for (int32_t i = 0; i < (field.width * field.length); i++){
        if (data[i] < currentMin){
            currentMin = data[i];
        } else if (data[i] > currentMax){
            currentMax = data[i];
        }
    }
    log_trace("currentMin: %.2f, currentMax: %.2f", currentMin, currentMax);

    // scale each value
    for (int32_t i = 0; i < (field.width * field.length); i++){
        double val = data[i];
        double scaled = (val - currentMin) / (currentMax - currentMin);
        outImage[i] = (uint8_t) (scaled * 255);
    }

    // render image
    stbi_write_bmp("../objective_function.bmp", field.length, field.width, 1, outImage);

    printf("total points: %d\n", totalPoints);
    puts("written image, quitting");
    exit(EXIT_SUCCESS);
}

/**
 * This function is just a wrapper which internally calls objective_func_impl() so that it can be used in both the
 * optimiser and the objective function renderer.
 * @param n the number of dimensions, should be always 2
 * @param x an N dimensional array containing the input values
 * @param grad the gradient, ignored as we're using a derivative-free optimisation algorithm
 * @param f_data user data, ignored
 * @return a score of how close the estimated position is to the real position
 */
static double objective_function(unsigned n, const double* x, double* grad, void* f_data){
    return objective_func_impl(x[0], x[1]);
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
        struct vec2 frameMin = {0, 0};
        struct vec2 frameMax = {entry->width, entry->height};

        // 1.1. cast line segments from the centre of the mirror to the edge of it, to calculate the line points (NOLINTNEXTLINE)
        for (double angle = 0.0; angle < PI2; angle += interval){
            double r = (double) visionMirrorRadius;
            double x1 = x0 + (r * cos(angle));
            double y1 = y0 + (r * sin(angle));
            raycast(entry->frame, ROUND2INT(x0), ROUND2INT(y0), ROUND2INT(x1), ROUND2INT(y1), entry->width, frameMin, frameMax, &linePoints);
        }

        // don't bother localising if no lines are present
        if (da_count(linePoints) == 0){
            goto cleanup;
        }

        // image normalisation
        // 2. dewarp points based on calculated mirror model to get centimetre field coordinates, then rotate based on robot's real orientation
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
//        double resultCoord[2] = {0.0, 0.0}; // TODO use last mouse sensor coordinate to seed optimisation
//        double resultError = 0.0;
//        nlopt_result result = nlopt_optimize(optimiser, resultCoord, &resultError);
//        if (result < 0){
//            // seem as though we've been stitched up and the NLopt version Ubuntu ships doesn't support nlopt_result_to_string()
//            log_warn("The optimiser may have failed to converge on a solution: status %s", nlopt_result_to_string(result));
//        }
//        printf("optimiser done with coordinate: %.2f,%.2f, error: %.2f, result id: %s\n", resultCoord[0], resultCoord[1],
//               resultError, nlopt_result_to_string(result));
//        localisedPosition.x = resultCoord[0];
//        localisedPosition.y = resultCoord[1];


        puts("calculating...");
        render_test_image();

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
    da_free(objectivePoints);
}