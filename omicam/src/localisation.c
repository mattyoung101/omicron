/*
 * This file is part of the Omicam project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
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
#include "qdbmp.h"
#include "movavg.h"
#include "remote_debug.h"
#include "errno.h"
#include "comms_uart.h"
#include "protobuf/Replay.pb.h"
#include "replay.h"

// This file implements our novel localisation algorithm that uses a hybrid sensor-fusion/non-linear optimisation approach,
// our statistical robot detection strategy, and finally comms with the remote debug UI (Omicontrol).
// For more information, see the page on our documentation website: https://teamomicron.github.io/omicam/

// ~~~~~ List of things to potentially investigate: ~~~~~
// - could we turn this into a non-linear least squares problem and solve with gauss-newton or levenberg-marquardt
// alternatively, we could just give the gradient to a gradient based optimisation algorithm
// - can/should we rewrite the objective function to calculate the R^2 value of the fit and maximise it?
// - look into proper DSP like low pass filter for smoothing instead of moving average
// - cap localiser to 24 Hz
// - dynamically change number of rays we cast depending on desired accuracy and error and stuff? in areas that are close
// to us, we can cast less rays, and in far away ones, we can cast more
// - cache or precalculate field file rays. got to think of a fast way (faster than hashmap even) to look em up

typedef struct {
    // begin and end indexes in ray list
    uint16_t begin;
    uint16_t end;
} ray_cluster_t;

DA_TYPEDEF(ray_cluster_t, ray_cluster_list_t)
DA_TYPEDEF(uint16_t, u16_list_t)
DA_TYPEDEF(struct vec2, vec2_list_t)

/** points visited by the Subplex optimiser */
lp_list_t localiserVisitedPoints = {0};
/** the bread and butter of the whole operation: NLopt!! */
static nlopt_opt optimiser;
static pthread_t workThread;
static FieldFile field = {0};
// we could probably just handle this with a single pthread condition and mutex, but just in case the localiser is too slow
// I want it to be able to support a backlog as well (otherwise we'd have to block the main thread until the last frame
// finishes optimising)
static rpa_queue_t *queue;
_Atomic bool localiserDone = false;
#if LOCALISER_DEBUG
static BMP *bmp = NULL;
#endif
/** rays that we observed from raycasting on the camera, with dewarping */
static double observedRays[LOCALISER_NUM_RAYS] = {0};
/** rays that we observed from raycasting on the camera image, no dewarping */
static double observedRaysRaw[LOCALISER_NUM_RAYS] = {0};
/** error value for the last group of rays evaluated by the objective function */
static double rayScores[LOCALISER_NUM_RAYS] = {0};
/** true or false if each ray is suspicious */
static bool suspiciousRays[LOCALISER_NUM_RAYS] = {0};
/** cache of previously computed rays for field file (expected rays), this is a 3D array of dimensions field length x field width x num rays */
static double *rayCache = NULL;
/** true if the given field cell has a cache entry associated with it, this is a 2D array of field length vs field width */
static bool *rayCacheWritten = NULL;

/** NLopt status for remote debug */
static char localiserStatus[32] = {0};
static pthread_t perfThread;
/** performance monitor moving averages */
static movavg_t *timeAvg = NULL, *evalAvg = NULL;
/** x and y position moving averages */
static movavg_t *xAvg = NULL, *yAvg = NULL;

/** last number of objective function evaluations */
static int32_t evaluations = 0;
pthread_mutex_t localiserMutex = PTHREAD_MUTEX_INITIALIZER;
static int32_t localiserRate = 0;
/** average number of localiser evaluations */
static int32_t localiserEvals = 0;
static int32_t cacheHits = 0, cacheMisses = 0;
static movavg_t *cacheMissesAverage = NULL;
static bool isGoalEstimateAvailable = false;

static struct vec2 estimateMinBounds = {0};
static struct vec2 estimateMaxBounds = {0};
static struct vec2 initialEstimate = {0};
static bool goalWasUnavailable = false;

/** this is equivalent to OBSDETECT_SUS_IQR_MUL * IQR */
static float susRayCutoff = 0.0f;
static RDObstacle detectedObstacles[4] = {0};
static int32_t detectedObstaclesCount = 0;

/**
 * Returns the index for the given entry in the ray cache array.
 * Source: https://stackoverflow.com/a/2306188/5007892
 */
static inline uint32_t rayCacheIndex(int32_t x, int32_t y, int32_t z){
    // x size is field.length, y size is field.width, z size is LOCALISER_NUM_RAYS
    return (z * field.length * field.width) + (y * field.length) + x;
}

static inline uint32_t rayCacheWrittenIndex(int32_t x, int32_t y){
    return x + field.length * y;
}

/**
 * Uses trigonometry to cast a ray from (x0, y0) until it either goes out of bounds (via minCorner, maxCorner or earlyStop) or
 * when it hits a line.
 * @param image the dataset we're raycasting on, assuming 0 is a line and anything not 0 is a line
 * @param x0 the origin x to start the ray from
 * @param y0 the origin y to start the ray from
 * @param theta angle in radians to cast ray along
 * @param imageWidth width of one line in the image
 * @param earlyStop if not -1, stop the raycasting early when the length exceeds this value (useful for stopping on mirror)
 * @param minCorner bottom left corner of image bounds rectangle
 * @param maxCorner top right corner of image bounds rectangle
 * @return the length of the ray
*/
static int32_t raycast(const uint8_t *image, int32_t x0, int32_t y0, double theta, int32_t imageWidth, int32_t earlyStop,
        struct vec2 minCorner, struct vec2 maxCorner){
    int32_t dist = 0;
#if LOCALISER_DEBUG
    int32_t r = rand() % 256; // NOLINT
    int32_t g = rand() % 256; // NOLINT
    int32_t b = rand() % 256; // NOLINT
#endif

    while (true){
        // EXTREMELY IMPORTANT NOTE: you will observe here the adding and subtracting of PI/2. initially, I accidentally
        // had this in the wrong direction (using sin then cos instead of cos then sin), but despite being a stupid mistake
        // it was somehow helpful, this is because the robot's default rotation frame of reference is facing the positive
        // X axis, or horizontal. anyways, this is mad confusing, but the important takeaways here are that
        // A) the robot faces horizontally on the X axis; and that B) I'm bad at maths
        int32_t rx = ROUND2INT(x0 + (dist * cosf(theta - PI / 2.0)));
        int32_t ry = ROUND2INT(y0 + (dist * sinf(theta + PI / 2.0)));
        // note on the above as well:
        // this trig here is one of the slowest parts of the objective function, we could speed it up by using an integer
        // approximate for sin and cos (the 32 bit float functions suffice here instead), or used Bresenham's algorithm
        // we could also cache these values maybe?

        // return if going to be out of bounds, and mark as ignored if so
        if (rx <= minCorner.x || ry <= minCorner.y || rx >= maxCorner.x || ry >= maxCorner.y){
            return -1;
        }
        // check if we exited mirror bounds, and mark as ignored if so
        if (earlyStop != -1 && dist > earlyStop){
            return -1;
        }

        // also return if we touch a line, or exited the early stop bounds
        int32_t i = rx + imageWidth * ry;
        if (image[i] != 0){
#if LOCALISER_DEBUG
            // we hit, go redraw the whole line for debug
            // this is required so we don't redraw ignored lines
            if (bmp != NULL){
                for (int j = 0; j < dist; j++){
                    int32_t frx = ROUND2INT(x0 + (j * sin(theta)));
                    int32_t fry = ROUND2INT(y0 + (j * cos(theta)));
                    BMP_SetPixelRGB(bmp, frx, fry, r, g, b);
                }
            }
#endif
            return dist;
        } else {
            dist++;
        }
    }
}

/**
 * This function will be minimised by the Subplex optimiser. For the given (x,y) coordinate it calculates the difference
 * between the observed rays and the expected rays at that coordinate. By minimising this function, you get the location
 * on the field with the least error between expected and observed rays - which is where we should be!
 * @param x the guessed x position in field coordinates
 * @param y the guessed y position in field coordinates
 * @return a score of how close the estimated position is to the real position
 */
static inline double objective_func_impl(double x, double y){
    double interval = PI2 / LOCALISER_NUM_RAYS;
    double x0 = x;
    double y0 = y;
    int32_t ix0 = ROUND2INT(x0);
    int32_t iy0 = ROUND2INT(y0);
    // note that these are in field file coordinates, not localiser coordinates (the ones with 0,0 as the field centre)
    struct vec2 minCoord = {0, 0};
    struct vec2 maxCoord = {field.length, field.width};
    double expectedRays[LOCALISER_NUM_RAYS] = {0};

    // raycast on our virtual field file to generate expected rays
    if (rayCacheWritten[rayCacheWrittenIndex(ix0, iy0)]){
        // use cached data
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
            expectedRays[i] = rayCache[rayCacheIndex(ix0, iy0, i)];
        }
        cacheHits++;
    } else {
        // no value exists for this cell in the cache yet, so write it
        double angle = 0.0;
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++) {
            double value = (double) raycast(field.data.bytes, X_TO_FF(x0), Y_TO_FF(y0), angle, field.length, -1, minCoord, maxCoord);
            expectedRays[i] = value;
            rayCache[rayCacheIndex(ix0, iy0, i)] = value;
            angle += interval;
        }
        cacheMisses++;
        rayCacheWritten[rayCacheWrittenIndex(ix0, iy0)] = true;
    }

    // compare ray lengths
    double totalError = 0.0;
    for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
        // the ray didn't land, so ignore it
        if (observedRays[i] <= 0 || expectedRays[i] <= 0){
            rayScores[i] = -1;
            continue;
        }

        // TODO make this the R^2 value, or alternatively or L2 loss function (quadratic error)
        double diff = fabs(expectedRays[i] - observedRays[i]);
        totalError += diff;
        rayScores[i] = diff;
    }
    evaluations++;

    localiser_point_t point = {x - field.length / 2.0, y - field.width / 2.0};
    pthread_mutex_lock(&localiserMutex);
    da_add(localiserVisitedPoints, point);
    pthread_mutex_unlock(&localiserMutex);

    return totalError;
}

#if LOCALISER_DEBUG
/** Renders the objective function for the whole field and then quits the application **/
static void render_test_image(){
    // render raycast
    bmp = BMP_Create(field.length, field.width, 24);
    objective_func_impl(200.0, 45.0);

    // render the field as well
    for (int y = 0; y < field.width; y++){
        for (int x = 0; x < field.length; x++){
            bool isLine = field.data.bytes[x + field.length * y] != 0;
            if (isLine){
                BMP_SetPixelRGB(bmp, x, y, 0, 0, 255);
            }
        }
    }
    BMP_WriteFile(bmp, "../raycast_debug.bmp");

    double *data = calloc(field.length * field.width, sizeof(double));
    uint8_t *outImage = calloc(field.length * field.width, sizeof(uint8_t));

    // calculate objective function for all data points
    printf("rendering objective function...\n");
    for (int32_t y = 0; y < field.width; y++){
        for (int32_t x = 0; x < field.length; x++){
            /*if (isGoalEstimateAvailable && (y <= estimateMinBounds.y || y >= estimateMaxBounds.y || x <= estimateMinBounds.x|| x >= estimateMaxBounds.x)){
                data[x + field.length * y] = LOCALISER_LARGE_ERROR; // kinda hacky, we do this to force the pixel to be black
                continue;
            }*/
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
        outImage[i] = 255 - (uint8_t) (scaled * 255);
    }

    // render image
    stbi_write_bmp("../objective_function.bmp", field.length, field.width, 1, outImage);

    puts("written image, quitting");
    exit(EXIT_SUCCESS);
}
#endif

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

/** The worker thread for the localiser, this is where the main code for it is */
static void *work_thread(void *arg){
    log_trace("Localiser work thread started");
    while (true){
        pthread_testcancel();
        void *queueData = NULL;
        if (!rpa_queue_pop(queue, &queueData)){
            log_warn("Failed to pop item from localisation work queue");
            continue;
        }
        double begin = utils_time_millis();
        localiser_entry_t *entry = (localiser_entry_t*) queueData;

        localiserDone = false;
        pthread_mutex_lock(&localiserMutex);
        da_clear(localiserVisitedPoints);
        pthread_mutex_unlock(&localiserMutex);

        // initial estimate calculation, currently using goals (as mouse sensor is unavailable)
        // see the file Goal maths.png (done by Ethan, thanks) in the docs folder for an explanation on how this works
        struct vec2 h1 = {91.5, 0}; // blue goal field position
        struct vec2 h2 = {-91.5, 0}; // yellow goal field position
        struct vec2 vb = svec2_subtract(h1, entry->blueRel);
        struct vec2 vy = svec2_subtract(h2, entry->yellowRel);
        struct vec2 numerator = svec2_add(svec2_multiply_f(vb, svec2_length(vy)), svec2_multiply_f(vy, svec2_length(vb)));
        double denominator = svec2_length(vy) + svec2_length(vb);
        initialEstimate = svec2_divide_f(numerator, denominator);

        // generate min and max bounds, being very careful not to clip out of bounds
        // also, we have to convert back to screen coordinates (origin is top left corner) since the optimiser needs that
        double hx = field.length / 2.0;
        double hy = field.width / 2.0;
        estimateMinBounds.x = constrainf(initialEstimate.x - LOCALISER_ESTIMATE_BOUNDS + hx, 0.0, field.length);
        estimateMinBounds.y = constrainf(initialEstimate.y - LOCALISER_ESTIMATE_BOUNDS + hy, 0.0, field.width);

        estimateMaxBounds.x = constrainf(initialEstimate.x + LOCALISER_ESTIMATE_BOUNDS + hx, 0.0, field.length);
        estimateMaxBounds.y = constrainf(initialEstimate.y + LOCALISER_ESTIMATE_BOUNDS + hy, 0.0, field.width);

        // handle cases if one/both of the goals aren't visible so we can't get a goal estimate
        isGoalEstimateAvailable = entry->yellowVisible && entry->blueVisible;
        if (isGoalEstimateAvailable && goalWasUnavailable){
            log_debug("Goals have returned, goal estimate available again");
            goalWasUnavailable = false;
        }
        if (!isGoalEstimateAvailable){
            // use mouse sensor data instead - which currently we don't have (so basically do nothing)
            // TODO use mouse sensor data by default (prioritise over goals) once UART and mouse sensor both work
            initialEstimate.x = lastSensorData.absDspX;
            initialEstimate.y = lastSensorData.absDspY;

            if (!goalWasUnavailable) {
                log_warn("Goal estimate will not be valid (yellow exists: %s, blue exists: %s)",
                         entry->yellowVisible ? "YES" : "NO", entry->blueVisible ? "YES" : "NO");
                // if the last time we received a UART packet was more than 100ms ago, warn the data is invalid
                double currentTime = utils_time_millis();
                if (currentTime - lastUARTReceiveTime >= 100){
                    log_warn("Mouse sensor data is stale (last received %.2f ms ago)", currentTime - lastUARTReceiveTime);
                }
                goalWasUnavailable = true;
            }
        }

        // image analysis
        // cast rays out from the centre of the camera frame, and record the length until it hits the white line
        double interval = PI2 / LOCALISER_NUM_RAYS;
        double x0 = entry->width / 2.0;
        double y0 = entry->height / 2.0;
        struct vec2 frameMin = {0, 0};
        struct vec2 frameMax = {entry->width, entry->height};

        // cast out our rays, stopping if we exceed the mirror bounds, and add to the raw observed rays list
        // start at the angle indicated by the IMU to account for robot orientation
        double angle = lastSensorData.orientation * DEG_RAD;
        for (int32_t i = 0; i < LOCALISER_NUM_RAYS; i++) {
            observedRaysRaw[i] = (double) raycast(entry->frame, ROUND2INT(x0), ROUND2INT(y0), angle, entry->width,
                    visionMirrorRadius, frameMin, frameMax);
            angle += interval;
        }

        // image normalisation
        // dewarp ray lengths based on mirror model to be in centimetres, then rotate based on BNO-055 reported orientation
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
            if (observedRaysRaw[i] <= 0){
                observedRays[i] = -1;
            } else {
                observedRays[i] = utils_camera_dewarp(observedRaysRaw[i]);
            }
        }

        // coordinate optimisation
        // using NLopt's Subplex optimiser, solve a 2D non-linear optimisation problem to calculate the optimal point
        // where the observed rays matches the expected rays the closest, see objective function for more info
        evaluations = 0;
        double optimiserPos[2] = {field.length / 2.0, field.width / 2.0};
        double optimiserError = 0.0;

        if (isGoalEstimateAvailable){
            // if we have a goal estimate, use that as our origin; otherwise, we will just use the field centre
            optimiserPos[0] = constrainf(initialEstimate.x + (field.length / 2.0), 0.0, field.length);
            optimiserPos[1] = constrainf(initialEstimate.y + (field.width / 2.0), 0.0, field.width);

            // also, if we have an estimate, use a smaller initial step size to try and converge faster
            nlopt_set_initial_step1(optimiser, LOCALISER_SMALL_STEP);

            // finally, set the optimiser bounds to our estimate bounds
            double min[2] = {estimateMinBounds.x, estimateMinBounds.y};
            double max[2] = {estimateMaxBounds.x, estimateMaxBounds.y};
            nlopt_set_lower_bounds(optimiser, min);
            nlopt_set_upper_bounds(optimiser, max);
        } else {
            // otherwise, use the default NLopt heuristically calculated initial step size
            nlopt_set_initial_step(optimiser, NULL);

            // and reset to default bounds
            double min[2] = {0.0, 0.0};
            double max[2] = {field.length, field.width};
            nlopt_set_lower_bounds(optimiser, min);
            nlopt_set_upper_bounds(optimiser, max);
        }
        nlopt_result result = nlopt_optimize(optimiser, optimiserPos, &optimiserError);
        if (result < 0){
            log_warn("NLopt error: status %s", nlopt_result_to_string(result));
            log_debug("Diagnostics: Min bounds: %.2f,%.2f, Max bounds: %.2f,%.2f, Estimate pos: %.2f,%.2f\n", estimateMinBounds.x,
                    estimateMinBounds.y, estimateMaxBounds.x, estimateMaxBounds.y, initialEstimate.x, initialEstimate.y);
        }
        // convert to field coordinates (where 0,0 is the centre of the field)
        optimiserPos[0] -= field.length / 2.0;
        optimiserPos[1] -= field.width / 2.0;

#if LOCALISER_ENABLE_SMOOTHING
        movavg_push(xAvg, optimiserPos[0]);
        movavg_push(yAvg, optimiserPos[1]);

#if LOCALISER_SMOOTHING_MEDIAN
        localisedPosition.x = movavg_calc_median(xAvg);
        localisedPosition.y = movavg_calc_median(yAvg);
#else
        localisedPosition.x = movavg_calc(xAvg);
        localisedPosition.y = movavg_calc(yAvg);
#endif
#else
        localisedPosition.x = optimiserPos[0];
        localisedPosition.y = optimiserPos[1];
#endif
        // FIXME maybe move all the obstacle detection crap to another file?? obsdetect.c
        // obstacle detection
        // sort rays by value, smallest to largest, for later IQR calculation
        double rayScoresSort[LOCALISER_NUM_RAYS] = {0};
        memcpy(rayScoresSort, rayScores, LOCALISER_NUM_RAYS * sizeof(double));
        qsort(rayScoresSort, LOCALISER_NUM_RAYS, sizeof(double), double_cmp);

        // calculate the inter-quartile range for use in detecting outliers (thanks to angus scroggie for the idea)
        // reference: https://brilliant.org/wiki/data-interquartile-range/
        // NOTE: to ease calculation, we make the (terrible?) assumption that LOCALISER_NUM_RAYS is even
        double q1tmp = 0.0;
        double q1f = modf((LOCALISER_NUM_RAYS + 1.0) / 4.0, &q1tmp);
        int32_t q1i = (int32_t) q1tmp;
        double q1 = rayScoresSort[q1i] + q1f * (rayScoresSort[q1i + 1] - rayScoresSort[q1i]);

        double q3tmp = 0.0;
        double q3f = modf(3.0 * ((LOCALISER_NUM_RAYS + 1.0) / 4.0), &q3tmp);
        int32_t q3i = (int32_t)  q3tmp;
        double q3 = rayScoresSort[q3i] + q3f * (rayScoresSort[q3i + 1] - rayScoresSort[q3i]);

        double iqr = q3 - q1;
        susRayCutoff = OBSDETECT_SUS_IQR_MUL * iqr;
//        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
//            printf("%f,", rayScoresSort[i]);
//        }
//        puts("");
//        printf("q1: %f, q3: %f, IQR: %f\n", q1, q3, iqr);

        // calculate the mean length
        double mean = 0.0;
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
            if (observedRays[i] > 0){
                mean += observedRays[i];
            }
        }
        mean /= (double) LOCALISER_NUM_RAYS;

        // flag suspicious rays (outliers with high error that have a smaller length than the mean)
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
            suspiciousRays[i] = rayScores[i] >= susRayCutoff && observedRays[i] <= mean && rayScores[i] > 0;
//            if (suspiciousRays[i]){
//                printf("ray %d is suspicious\n", i);
//            }
        }

        // try and create clusters of suspicious rays taking into account the tolerance
        ray_cluster_list_t rayClusters = {0};
        bool isClustering = false;
        ray_cluster_t currentCluster = {0};
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
            if (suspiciousRays[i] && !isClustering){
                // if we have a suspicious ray, and are not currently clustering, start a new cluster
                memset(&currentCluster, 0, sizeof(ray_cluster_t));
                currentCluster.begin = i;
                isClustering = true;
            } else if (!suspiciousRays[i]) {
                // the current ray is non suspicious: check if the rays are all non suspicious after the current one
                // TODO maybe should do for loop and if they're ALL non suspicious after RAY_GROUP_TOLERANCE terminate otherwise not
                uint16_t nextIndex = constrain(i + OBSDETECT_RAY_GROUP_TOLERANCE, 0, LOCALISER_NUM_RAYS - 1);

                if (!suspiciousRays[nextIndex] && isClustering){
                    // likely no more rays, terminate cluster and add to list
                    currentCluster.end = i;
                    isClustering = false;
                    da_add(rayClusters, currentCluster);
                } else {
                    // cluster keeps on going, or was not started in the first place, either way do nothing
                }
            }
        }

        // prune clusters which are too small, and join up ones at the end and beginning
        // separate list of removal indices to prevent concurrent modification problems
        u16_list_t removeIndexes = {0};
        for (size_t i = 0; i < da_count(rayClusters); i++){
            ray_cluster_t cluster = da_get(rayClusters, i);
            if (abs(cluster.end - cluster.begin) < OBSDETECT_MIN_CLUSTER_SIZE){
                // printf("cluster %zu from %d-%d is too small, purging\n", i, cluster.begin, cluster.end);
                da_add(removeIndexes, i);
            }
        }
        uint16_t removedCount = 0;
        for (size_t i = 0; i < da_count(removeIndexes); i++){
            uint16_t index = da_get(removeIndexes, i) - removedCount;
            da_delete(rayClusters, index);
            removedCount++;
        }

        // TODO join up clusters at the end of the robot (at like n = 64) to the beginning

        // for each scalene triangle defined by begin and end ray angles and the distance to the closest field line,
        // calculate whether or not a robot could possibly fit in it
        // thanks again to angus scroggie for coming up with the idea and maths for this and the next step
        struct vec2 minCoord = {0, 0};
        struct vec2 maxCoord = {field.length, field.width};
        vec2_list_t detectedRobots = {0};
        memset(detectedObstacles, 0, 4 * sizeof(RDObstacle));
        detectedObstaclesCount = 0;

        for (size_t i = 0; i < da_count(rayClusters); i++){
            ray_cluster_t cluster = da_get(rayClusters, i);

            // cast from the midpoint of the rays to the field line to form the end of the triangle
            // we also have to convert back from centre coordinates to top left cooridnates
            uint16_t midpoint = (cluster.begin + cluster.end) / 2;
            double midpointAngle = midpoint * interval;
            // robot position in field file coords
            double rx = X_TO_FF(localisedPosition.x + field.length / 2.0);
            double ry = Y_TO_FF(localisedPosition.y + field.width / 2.0);
            // note: in this raycast, it will terminate on the goalie box on the ints field which may cause problems
            // if robots are in there. however, I'm going to make the assumption here that there will be no illegal robots
            int32_t midpointLength = raycast(field.data.bytes, rx, ry, midpointAngle, field.length, -1, minCoord, maxCoord);

            // now find the area of the triangle, to find the length of the base we find the two coords that define the
            // base line and euclidean distance between them, so we raycast out from the robot to the midpoint length
            // IMPORTANT NOTE: we intentionally use trig the wrong way around here, see raycast() for details
            double firstX = localisedPosition.x + midpointLength * sin(cluster.begin * interval);
            double firstY = localisedPosition.y + midpointLength * cos(cluster.begin * interval);
            double lastX = localisedPosition.x + midpointLength * sin(cluster.end * interval);
            double lastY = localisedPosition.y + midpointLength * cos(cluster.end * interval);
            double base = sqrt(pow(firstX - lastX, 2) + pow(firstY - lastY, 2));
            double triArea = base * midpointLength / 2.0;
            // printf("triangle area: %.2f\n", triArea);

            double sideA = sqrt(pow(rx - firstX, 2) + pow(ry - firstY, 2));
            double sideB = sqrt(pow(rx - lastX, 2) + pow(ry - lastY, 2));
            double triPerimeter = sideA + sideB + base;
            // this is the maximum radius of a circle that can be inscribed in our triangle
            double maxRadius = (triArea / triPerimeter) * 2.0;
            // printf("perimeter: %.2f, max radius: %.2f\n", triPerimeter, maxRadius);

            // finally, check if the max robot size permits a robot to be in this cluster
            if (OBSDETECT_MAX_ROBOT_DIAMETER <= maxRadius * 2){
                // if it does, calculate where the robot might be. we can't actually know where the robot is exactly,
                // (at least I think not), so we assume that the detected robot is as close as possible to our
                // robot without going outside the cluster bounds.
                // this is implemented as defined by angus' desmos link: https://www.desmos.com/calculator/0oaombib4e
                double a = tan(cluster.begin * interval - PI / 2);
                double b = tan(cluster.end * interval - PI / 2);
                if (a <= 0){
                    // we have to swap, so the hack doesn't break
                    double oldA = a;
                    a = b;
                    b = oldA;
                }
                double t = tan((atan(a) + atan(b)) / 2.0);
                double R = OBSDETECT_MAX_ROBOT_DIAMETER / 2.0;
                double invA = pow(a, -1);
                double c = (R * (t + invA) * (a + invA)) / (sqrt(pow(t * invA - 1, 2) + pow(a - t, 2)));

                // final coords of robot: (C, D), relative to our current position
                double C = (c) / (t + invA);
                double D = t * C;
                double finalX = localisedPosition.x + C;
                // due to yet more inane coordinate system craziness (likely the stupid thing being Y-down
                // for some reason), we have to SUBTRACT this idiot to make it line up properly
                // someone please make me fix all these coordinate systems, seriously, why did I make it Y-down?????
                double finalY = localisedPosition.y - D;

                // record debug information (very messy code... yikes!)
                detectedObstacles[detectedObstaclesCount].susTriBegin.x = (float) firstX;
                detectedObstacles[detectedObstaclesCount].susTriBegin.y = (float) firstY;
                detectedObstacles[detectedObstaclesCount].susTriEnd.x = (float) lastX;
                detectedObstacles[detectedObstaclesCount].susTriEnd.y = (float) lastY;
                detectedObstacles[detectedObstaclesCount].centroid.x = (float) finalX;
                detectedObstacles[detectedObstaclesCount].centroid.y = (float) finalY;
                detectedObstaclesCount++;

                struct vec2 detectedRobot = {finalX, finalY};
                da_add(detectedRobots, detectedRobot);
                // printf("final robot coords: %.2f,%.2f\n", finalX, finalY);
            }
        }
        // TODO do something with detected robots here (we need to add them to the UART message)

        // serialisation stage
        // dispatch data to the ESP32, using JimBus (Protobufs over UART)
        uint8_t msgBuf[128] = {0};
        pb_ostream_t stream = pb_ostream_from_buffer(msgBuf, 128);
        LocalisationData localisationData = LocalisationData_init_zero;
        localisationData.estimatedX = (float) localisedPosition.x;
        localisationData.estimatedY = (float) localisedPosition.y;

        if (!pb_encode(&stream, LocalisationData_fields, &localisationData)){
            log_error("Failed to encode localisation Protobuf message: %s", PB_GET_ERROR(&stream));
        }
        comms_uart_send(MSG_LOCALISATION_DATA, msgBuf, stream.bytes_written);

#if LOCALISER_DEBUG
        printf("optimiser done with coordinate: %.2f,%.2f, error: %.2f, result id: %s\n", optimiserPos[0], optimiserPos[1],
               optimiserError, nlopt_result_to_string(result));

        // debug, only for use when robot is at a known (0,0) position!!
        // double accuracy = sqrt(optimiserPos[0] * optimiserPos[0] + optimiserPos[1] * optimiserPos[1]);
        // printf("accuracy: %.2f (pos: %.2f,%.2f)\n", accuracy, optimiserPos[0], optimiserPos[1]);

        log_info("Localisation debug enabled, displaying objective function and quitting...");
        render_test_image();
#endif
        // dispatch information to performance monitor and Omicontrol, then clean up
        double elapsed = utils_time_millis() - begin;
        movavg_push(timeAvg, elapsed);
        movavg_push(evalAvg, evaluations);
        double cacheMissesPercentage = (double) cacheMisses / (cacheHits + cacheMisses);
        movavg_push(cacheMissesAverage, cacheMissesPercentage);
        cacheHits = 0;
        cacheMisses = 0;

        const char *resultStr = nlopt_result_to_string(result);
        memset(localiserStatus, 0, 32);
        strncpy(localiserStatus, resultStr, 32);

        // dispatch to replay file, if one is recording
        ReplayFrame replay = ReplayFrame_init_zero;
        replay.robots[0].position.x = localisedPosition.x;
        replay.robots[0].position.y = localisedPosition.y;
        // TODO set robot orientation and fsm state here once we get it from UART
        replay.robots_count = 1;

        replay.isYellowKnown = entry->objectData.goalYellowExists;
        replay.isBlueKnown = entry->objectData.goalBlueExists;
        replay.isBallKnown = entry->objectData.ballExists;
        if (replay.isYellowKnown) {
            replay.yellowGoalPos.x = entry->objectData.goalYellowAbsX;
            replay.yellowGoalPos.y = entry->objectData.goalYellowAbsY;
        }
        if (replay.isBlueKnown){
            replay.blueGoalPos.x = entry->objectData.goalBlueAbsX;
            replay.blueGoalPos.y = entry->objectData.goalBlueAbsY;
        }
        if (replay.isBallKnown){
            replay.ballPos.x = entry->objectData.ballAbsX;
            replay.ballPos.y = entry->objectData.ballAbsY;
        }

        replay.estimateMaxBounds.x = estimateMaxBounds.x;
        replay.estimateMaxBounds.y = estimateMaxBounds.y;
        replay.estimateMinBounds.x = estimateMinBounds.x;
        replay.estimateMinBounds.y = estimateMinBounds.y;
        replay.localiserEvals = localiserEvals;
        replay.localiserRate = localiserRate;
        replay_post_frame(replay);

        da_free(rayClusters);
        da_free(removeIndexes);
        da_free(detectedRobots);
        free(entry->frame);
        free(entry);
        localiserDone = true;
        pthread_testcancel();
    }
    return NULL;
}

void remote_debug_localiser_provide(DebugFrame *msg){
    memcpy(msg->dewarpedRays, observedRays, LOCALISER_NUM_RAYS * sizeof(double));
    memcpy(msg->raysSuspicious, suspiciousRays, LOCALISER_NUM_RAYS * sizeof(bool));
    msg->dewarpedRays_count = LOCALISER_NUM_RAYS;
    msg->raysSuspicious_count = LOCALISER_NUM_RAYS;
    msg->susRayCutoff = susRayCutoff;

    memcpy(msg->rays, observedRaysRaw, LOCALISER_NUM_RAYS * sizeof(double));
    msg->rays_count = LOCALISER_NUM_RAYS;

    msg->robots[0].position.x = (float) localisedPosition.x;
    msg->robots[0].position.y = (float) localisedPosition.y;
    msg->robots_count = 1;

    memcpy(msg->detectedObstacles, detectedObstacles, detectedObstaclesCount * sizeof(RDObstacle));
    msg->detectedObstacles_count = detectedObstaclesCount;

    msg->localiserRate = localiserRate;
    msg->localiserEvals = localiserEvals;
    strncpy(msg->localiserStatus, localiserStatus, 32);
    msg->rayInterval = (float) (PI2 / LOCALISER_NUM_RAYS);

    // Omicontrol wants these in field coordinates, not screen coordinates
    float hx = field.length / 2.0f;
    float hy = field.width / 2.0f;
    msg->estimateMinBounds.x = (float) estimateMinBounds.x - hx;
    msg->estimateMinBounds.y = (float) estimateMinBounds.y - hy;
    msg->estimateMaxBounds.x = (float) estimateMaxBounds.x - hx;
    msg->estimateMaxBounds.y = (float) estimateMaxBounds.y - hy;

    // these are already in field coordinates
    msg->goalEstimate.x = (float) initialEstimate.x;
    msg->goalEstimate.y = (float) initialEstimate.y;

    // copy a max of 128 points over
    uint32_t size = MIN(da_count(localiserVisitedPoints), 128);
    for (size_t i = 0; i < size; i++) {
        localiser_point_t point = da_get(localiserVisitedPoints, i);
        msg->localiserVisitedPoints[i].x = (float) point.x;
        msg->localiserVisitedPoints[i].y = (float) point.y;
    }
    msg->localiserVisitedPoints_count = size;
}

/** localiser performance monitoring thread */
static void *perf_thread(void *arg){
    while (true){
        sleep(1);
        if (sleeping) continue;

        double avgTime = movavg_calc(timeAvg);
        double avgEval = movavg_calc(evalAvg);
        if (avgTime == 0) continue;

        double rate = (1000.0 / avgTime);
        localiserRate = ROUND2INT(rate);
        localiserEvals = ROUND2INT(avgEval);
        double avgCacheMisses = movavg_calc(cacheMissesAverage) * 100.0;

#if LOCALISER_DIAGNOSTICS
        if (REMOTE_ALWAYS_SEND || !remote_debug_is_connected()){
            printf("Localiser average evals: %d, time: %.2f ms, cache misses: %.2f%%, rate: %.2f Hz\n", ROUND2INT(avgEval),
                    avgTime, avgCacheMisses, rate);
            fflush(stdout);
        }
#endif
    }
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

    // allocate ray cache and ray written cache
    rayCache = calloc(field.length * field.width * LOCALISER_NUM_RAYS, sizeof(double));
    rayCacheWritten = calloc(field.length * field.width, sizeof(bool));

    // create a two dimensional minimising Subplex optimiser
    optimiser = nlopt_create(NLOPT_LN_SBPLX, 2);
    nlopt_set_min_objective(optimiser, objective_function, NULL);
    nlopt_set_stopval(optimiser, LOCALISER_ERROR_TOLERANCE); // stop if we're close enough to a solution (not very helpful now as error is arbitrary)
    nlopt_set_ftol_abs(optimiser, LOCALISER_STEP_TOLERANCE); // stop if the last step was too small (we must be going nowhere/solved)
    nlopt_set_maxtime(optimiser, LOCALISER_MAX_EVAL_TIME / 1000.0); // stop if it's taking too long

    // note we are in image coordinates (top left is 0,0), NOT field coordinates (centre is 0,0)
    double minCoord[] = {0.0, 0.0};
    double maxCoord[] = {field.length, field.width};
    log_trace("Initial min bound: (%.2f,%.2f), max bound: (%.2f,%.2f)", minCoord[0], minCoord[1], maxCoord[0], maxCoord[1]);
    nlopt_set_lower_bounds(optimiser, minCoord);
    nlopt_set_upper_bounds(optimiser, maxCoord);

    // create work thread
    rpa_queue_create(&queue, 1);
    int err = pthread_create(&workThread, NULL, work_thread, NULL);
    if (err != 0){
        log_error("Failed to create localiser thread: %s", strerror(err));
    } else {
        pthread_setname_np(workThread, "Localiser Thrd");
    }

    // create performance monitor thread
    timeAvg = movavg_create(256);
    evalAvg = movavg_create(256);
    cacheMissesAverage = movavg_create(256);
    xAvg = movavg_create(LOCALISER_SMOOTHING_SIZE);
    yAvg = movavg_create(LOCALISER_SMOOTHING_SIZE);
    err = pthread_create(&perfThread, NULL, perf_thread, NULL);
    if (err != 0){
        log_error("Failed to create localiser perf thread: %s", strerror(err));
    } else {
        pthread_setname_np(perfThread, "Localiser Perf");
    }

    pthread_mutex_init(&localiserMutex, NULL);
    pthread_mutex_unlock(&localiserMutex);
    free(data);
    pthread_testcancel();
}

void localiser_post(uint8_t *frame, int32_t width, int32_t height, struct vec2 yellowRel, struct vec2 blueRel, ObjectData objectData){
    localiser_entry_t *entry = malloc(sizeof(localiser_entry_t));
    entry->frame = frame;
    entry->width = width;
    entry->height = height;
    entry->yellowRel = yellowRel;
    entry->blueRel = blueRel;
    entry->yellowVisible = objectData.goalYellowExists;
    entry->blueVisible = objectData.goalBlueExists;
    entry->objectData = objectData;
    if (!rpa_queue_trypush(queue, entry)){
        // localiser is busy, drop frame
        free(frame);
        free(entry);
    }
}

void localiser_dispose(void){
    log_trace("Disposing localiser");
    pthread_cancel(workThread);
    pthread_cancel(perfThread);
    pthread_join(workThread, NULL);
    pthread_join(perfThread, NULL);
    nlopt_force_stop(optimiser);
    nlopt_destroy(optimiser);
    rpa_queue_destroy(queue);
    movavg_free(timeAvg);
    movavg_free(evalAvg);
    movavg_free(xAvg);
    movavg_free(yAvg);
    movavg_free(cacheMissesAverage);
    free(rayCache);
    free(rayCacheWritten);
    da_free(localiserVisitedPoints);
}
