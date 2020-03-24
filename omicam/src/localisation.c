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

lp_list_t localiserVisitedPoints = {0};
static nlopt_opt optimiser;
static pthread_t workThread;
static FieldFile field = {0};
// we could probably just handle this with a single pthread condition and mutex, but just in case the localiser is too slow
// I want it to be able to support a backlog as well (otherwise we'd have to block the main thread until the last frame
// finishes optimising)
static rpa_queue_t *queue;
_Atomic bool localiserDone = false;
static double orientation = 0.0; // last received orientation from ESP32
#if LOCALISER_DEBUG
static BMP *bmp = NULL;
#endif
double observedRays[LOCALISER_NUM_RAYS] = {0};
double observedRaysRaw[LOCALISER_NUM_RAYS] = {0};
double expectedRays[LOCALISER_NUM_RAYS] = {0};
double rayScores[LOCALISER_NUM_RAYS] = {0};
char localiserStatus[32] = {0};
static pthread_t perfThread;
static movavg_t *timeAvg = NULL;
static movavg_t *evalAvg = NULL;
static int32_t evaluations = 0;
pthread_mutex_t localiserMutex = PTHREAD_MUTEX_INITIALIZER;
int32_t localiserRate = 0;
int32_t localiserEvals = 0;

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
static int32_t raycast(const uint8_t *image, int32_t x0, int32_t y0, double theta, int32_t imageWidth, int32_t earlyStop, struct vec2 minCorner, struct vec2 maxCorner){
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
 * between the observed rays and the expected rays at that coordinate.
 * @param x the guessed x position in field coordinates
 * @param y the guessed y position in field coordinates
 * @return a score of how close the estimated position is to the real position
 */
static inline double objective_func_impl(double x, double y){
    // HACK HACK HACK: check if out of bounds and return a huge number if so
    if ((x < 28 || x > 215) || (y < 29 || y > 155)) {
        return 8600;
    }

    // 1. raycast out from the guessed x and y on the field file
    double interval = PI2 / LOCALISER_NUM_RAYS;
    double x0 = x;
    double y0 = y;

    // note that these are in field file coordinates, not localiser coordinates (the ones with 0,0 as the field centre)
    struct vec2 minCoord = {0, 0};
    struct vec2 maxCoord = {field.length, field.width};

    // 1.2. loop over the points and raycast
    double angle = 0.0;
    for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
        expectedRays[i] = (double) raycast(field.data.bytes, X_TO_FF(x0), Y_TO_FF(y0), angle, field.length, -1, minCoord, maxCoord);
        angle += interval;
    }

    // 2. compare ray lengths
    double totalError = 0.0;
    for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
        // the ray didn't land, so ignore it
        if (observedRays[i] == -1 || expectedRays[i] == -1) continue;

        double diff = fabs(expectedRays[i] - observedRays[i]);
        rayScores[i] = diff;
        totalError += diff;
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

    BMP_WriteFile(bmp, "../testing.bmp");

    double *data = calloc(field.length * field.width, sizeof(double));
    uint8_t *outImage = calloc(field.length * field.width, sizeof(uint8_t));

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

        double begin = utils_time_millis();
        localiser_entry_t *entry = (localiser_entry_t*) queueData;
        localiserDone = false;

        pthread_mutex_lock(&localiserMutex);
        da_clear(localiserVisitedPoints);
        pthread_mutex_unlock(&localiserMutex);

        // TODO we're going to have to reconstruct the localisation pipeline from scratch with this new hyrbid algorithm
        // but clearly the first thing we're going to want to localise with the goal positions only
        // then calculate which quadrant we're in, and somewhere in here feed back in the mouse sensor data once we have some

        // image analysis
        // 1. calculate begin points for rays
        double interval = PI2 / LOCALISER_NUM_RAYS;
        double x0 = entry->width / 2.0;
        double y0 = entry->height / 2.0;
        struct vec2 frameMin = {0, 0};
        struct vec2 frameMax = {entry->width, entry->height};

        // 1.1. cast rays from the centre of the mirror to the edge of it, to calculate the ray lengths
        double angle = 0.0;
        for (int32_t i = 0; i < LOCALISER_NUM_RAYS; i++) {
            observedRaysRaw[i] = (double) raycast(entry->frame, ROUND2INT(x0), ROUND2INT(y0), angle, entry->width,
                    visionMirrorRadius, frameMin, frameMax);
            angle += interval;
        }

        // image normalisation
        // 2. dewarp ray lengths based on calculated mirror model to get centimetre field coordinates, then rotate based on robot's real orientation
        for (int i = 0; i < LOCALISER_NUM_RAYS; i++){
            if (observedRays[i] == -1) continue;
            observedRays[i] = utils_camera_dewarp(observedRaysRaw[i]);
        }
        // TODO (!!IMPORTANT!!) somehow we also need to rotate this array based on orientation of robot

        // coordinate optimisation
        // 3. start the NLopt Subplex optimiser
        evaluations = 0;
        double resultCoord[2] = {field.width / 2.0, field.width / 2.0};
        double resultError = 0.0;
        nlopt_result result = nlopt_optimize(optimiser, resultCoord, &resultError);
        if (result < 0){
            log_warn("The optimiser may have failed to converge on a solution: status %s", nlopt_result_to_string(result));
        }
        resultCoord[0] -= field.length / 2.0;
        resultCoord[1] -= field.width / 2.0;
#if LOCALISER_DEBUG
        printf("optimiser done with coordinate: %.2f,%.2f, error: %.2f, result id: %s\n", resultCoord[0], resultCoord[1],
               resultError, nlopt_result_to_string(result));
#endif
        localisedPosition.x = resultCoord[0];
        localisedPosition.y = resultCoord[1];
        const char *resultStr = nlopt_result_to_string(result);
        memset(localiserStatus, 0, 32);
        memcpy(localiserStatus, resultStr, MIN(strlen(resultStr), 32));

        // 4. dispatch information to ESP32 firmware
        uint8_t msgBuf[128] = {0};
        pb_ostream_t stream = pb_ostream_from_buffer(msgBuf, 128);
        LocalisationData localisationData = LocalisationData_init_default;
        localisationData.estimatedX = (float) localisedPosition.x;
        localisationData.estimatedY = (float) localisedPosition.y;

        if (!pb_encode(&stream, LocalisationData_fields, &localisationData)){
            log_error("Failed to encode localisation Protobuf message: %s", PB_GET_ERROR(&stream));
        }
        comms_uart_send(LOCALISATION_DATA, msgBuf, stream.bytes_written);

        // dispatch information to performance monitor and clean up
#if LOCALISER_DEBUG
        log_info("Localisation debug enabled, displaying objective function and quitting...");
        render_test_image();
#endif
        double elapsed = utils_time_millis() - begin;
        movavg_push(timeAvg, elapsed);
        movavg_push(evalAvg, evaluations);

        free(entry->frame);
        free(entry);
        localiserDone = true;
        pthread_testcancel();
    }
    return NULL;
}

static void *perf_thread(void *arg){
    while (true){
        sleep(1);
        double avgTime = movavg_calc(timeAvg);
        double avgEval = movavg_calc(evalAvg);
        if (avgTime == 0) continue;

        double rate = (1000.0 / avgTime);
        localiserRate = ROUND2INT(rate);
        localiserEvals = ROUND2INT(avgEval);
        movavg_clear(timeAvg);
        movavg_clear(evalAvg);

#if LOCALISER_DIAGNOSTICS
        if (REMOTE_ALWAYS_SEND || !remote_debug_is_connected()){
            printf("Average localiser time: %.2f ms (rate: %.2f Hz), average evaluations: %d\n", avgTime, rate, ROUND2INT(avgEval));
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

    // create performance monitor thread
    timeAvg = movavg_create(256);
    evalAvg = movavg_create(256);
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

void localiser_post(uint8_t *frame, int32_t width, int32_t height, struct vec2 yellowGoal, struct vec2 blueGoal){
    localiser_entry_t *entry = malloc(sizeof(localiser_entry_t));
    entry->frame = frame;
    entry->width = width;
    entry->height = height;
    entry->yellowGoal = yellowGoal;
    entry->blueGoal = blueGoal;
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
    da_free(localiserVisitedPoints);
}