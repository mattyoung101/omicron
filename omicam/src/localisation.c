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

// This file implements our novel sensor-fusion localisation approach. It works by using generating an initial guess
// through traditional methods like vector maths on the goals and mouse sensor integration, but uniquely refines
// this inaccurate initial guess by solving a non-linear multi-variate optimisation problem in real-time. For more
// information, see our documentation website.

// TODO list of things to potentially investigate:
// - could we turn this into a non-linear least squares problem and solve with gauss-newton or levenberg-marquardt
// alternatively, we could just give the gradient to a gradient based optimisation algorithm
// - can/should we rewrite the objective function to calculate the R^2 value of the fit and maximise it?
// - look into proper DSP like low pass filter for smoothing instead of moving average
// - cap localiser to 24 Hz
// - (WIP) look into a way to detect robots (documented better on trello)
// - dynamically change number of rays we cast depending on desired accuracy and error and stuff? in areas that are close
// to us, we can cast less rays, and in far away ones, we can cast more

DA_TYPEDEF(uint16_t, u16_list_t)

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
/** NLopt status for remote debug */
static char localiserStatus[32] = {0};
static pthread_t perfThread;
static movavg_t *timeAvg = NULL;
static movavg_t *evalAvg = NULL;
static movavg_t *xAvg = NULL;
static movavg_t *yAvg = NULL;
static int32_t evaluations = 0;
pthread_mutex_t localiserMutex = PTHREAD_MUTEX_INITIALIZER;
static int32_t localiserRate = 0;
static int32_t localiserEvals = 0;
static bool isGoalEstimateAvailable = false;
static struct vec2 estimateMinBounds = {0};
static struct vec2 estimateMaxBounds = {0};
static struct vec2 initialEstimate = {0};
static bool goalWasUnavailable = false;
/** this is equivalent to OBSDETECT_SUS_IQR_MUL * IQR */
static float susRayCutoff = 0.0f;

static inline int32_t constrain(int32_t x, int32_t min, int32_t max){
    if (x < min){
        return min;
    } else if (x > max){
        return max;
    } else {
        return x;
    }
}

static inline double constrainf(double x, double min, double max){
    if (x < min){
        return min;
    } else if (x > max){
        return max;
    } else {
        return x;
    }
}

/** comparator used to sort an array of doubles with qsort() in ascending order (I think) */
static int double_cmp(const void *a, const void *b){
    double x = *(const double *)a;
    double y = *(const double *)b;

    if (x < y){
        return -1;
    } else if (x > y){
        return 1;
    } else {
        return 0;
    }
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
        int32_t rx = ROUND2INT(x0 + (dist * sin(theta)));
        int32_t ry = ROUND2INT(y0 + (dist * cos(theta)));

        // return if going to be out of bounds, and mark as ignored if so
        if (rx <= minCorner.x || ry <= minCorner.y || rx >= maxCorner.x || ry >= maxCorner.y){
            // the ray didn't land inside bounds so mark it as ignored
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
    // note that these are in field file coordinates, not localiser coordinates (the ones with 0,0 as the field centre)
    struct vec2 minCoord = {0, 0};
    struct vec2 maxCoord = {field.length, field.width};
    double expectedRays[LOCALISER_NUM_RAYS] = {0};

    // raycast on our virtual field file to generate expected rays
    double angle = 0.0;
    for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
        expectedRays[i] = (double) raycast(field.data.bytes, X_TO_FF(x0), Y_TO_FF(y0), angle, field.length, -1, minCoord, maxCoord);
        angle += interval;
    }

    // compare ray lengths
    double totalError = 0.0;
    for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
        // the ray didn't land, so ignore it
        if (observedRays[i] <= 0 || expectedRays[i] <= 0){
            rayScores[i] = -1;
            continue;
        }

        // TODO make this the R^2 value, or alternatively, sum of residuals (square output?)

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
            if (isGoalEstimateAvailable && (y <= estimateMinBounds.y || y >= estimateMaxBounds.y || x <= estimateMinBounds.x|| x >= estimateMaxBounds.x)){
                data[x + field.length * y] = LOCALISER_LARGE_ERROR; // kinda hacky, we do this to force the pixel to be black
                continue;
            }
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
        // obstacle detection
        // sort rays by value, smallest to largest, for later IQR calculation
        double rayScoresSort[LOCALISER_NUM_RAYS] = {0};
        memcpy(rayScoresSort, rayScores, LOCALISER_NUM_RAYS * sizeof(double));
        qsort(rayScoresSort, LOCALISER_NUM_RAYS, sizeof(double), double_cmp);

        // calculate the inter-quartile range for use in detecting outliers (thanks to angus scroggie for help with this)
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
        // TODO we could get rid of this dumb linked list and just use rdSusRays?
        u16_list_t susRaysList = {0};
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
            if (rayScores[i] >= susRayCutoff && observedRays[i] <= mean){
                da_add(susRaysList, i);
                suspiciousRays[i] = true;
                // printf("ray %d is suspicious\n", i);
            } else {
                suspiciousRays[i] = false;
            }
        }
        // printf("have: %zu suspicious rays\n", da_count(susRaysList));

        // try and create clusters of suspicious rays taking into account the tolerance
        // TODO when we do this we have to consider that ray 64 can join with ray 0, etc

        // filter out any that aren't robot shaped

        // record our obstacles

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

        da_free(susRaysList);
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

#if LOCALISER_DIAGNOSTICS
        if (REMOTE_ALWAYS_SEND || !remote_debug_is_connected()){
            //printf("Average localiser time: %.2f ms (rate: %.2f Hz), average evaluations: %d\n", avgTime, rate, ROUND2INT(avgEval));
            printf("Localiser average evals: %d, time: %.2f ms, rate: %.2f Hz\n", ROUND2INT(avgEval), avgTime, rate);
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
    da_free(localiserVisitedPoints);
}