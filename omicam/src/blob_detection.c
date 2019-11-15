#include "blob_detection.h"
#include "utils.h"
#include <math.h>
#include <stdlib.h>
#include <log/log.h>
#include <omp.h>
#include <rpa_queue.h>

// Rectangle code based on libGDX's Rectangle.java
// https://github.com/libgdx/libgdx/blob/master/gdx/src/com/badlogic/gdx/math/Rectangle.java
// eventually (if this is too slow, which it probably will be) this will use connected component labelling algorithms
// pthread condition example source: https://gist.github.com/rtv/4989304

static inline uint8_t imax8(uint8_t first, uint8_t second){
    return first > second ? first : second;
}

static inline uint8_t imin8(uint8_t first, uint8_t second){
    return first < second ? first : second;
}

#if !BLOB_USE_NEON
/**
 * Thresholds a colour, using scalar instructions (no SIMD/NEON)
 * @param value the colour in question, must be a 3 element array
 * @param min the minimum colour, 3 element array
 * @param max the maximum colour, 3 element array
 * @return whether or not colour is in range of min and max
 */
static inline __attribute__((hot)) bool in_range(const uint8_t *value, const uint8_t *min, const uint8_t *max){
//    bool red = value[R] >= min[R] && value[R] <= max[R];
//    bool green = value[G] >= min[G] && value[G] <= max[G];
//    bool blue = value[B] >= min[B] && value[B] <= max[B];
//    return red && green && blue;

    // based on OpenMV's threshold editor code:
    // https://github.com/openmv/qt-creator/blob/66f56c420e651ca5f0bfcf300ac826a7bb7c27c6/src/plugins/openmv/tools/thresholdeditor.cpp#L65
    // only based on, not copied from, so GPL does not apply
    bool rMinOk = value[R] >= imin8(min[R], max[R]);
    bool rMaxOk = value[R] <= imax8(max[R], min[R]);

    bool gMinOk = value[G] >= imin8(min[G], max[G]);
    bool gMaxOk = value[G] <= imax8(max[G], min[G]);

    bool bMinOk = value[B] >= imin8(min[B], max[B]);
    bool bMaxOk = value[B] <= imax8(max[B], min[B]);
    return (rMinOk && rMaxOk && gMinOk && gMaxOk && bMinOk && bMaxOk);
}
#else
/**
 * Thresholds a colour, using vectors instructions (SIMD/NEON)
 * @param value the colour in question, must be a 3 element array
 * @param min the minimum colour, vector should contain 3 elements (R, G, B)
 * @param max the maximum colour, vector should contain 3 elements (R, G, B)
 * @return whether or not colour is in range of min and max
 */
static inline bool in_range(uint8x8_t value, uint8x8_t min, uint8x8_t max){
    uint8x8_t inMin = vcge_u8(value, min); // "vector compare greater than or equal to"
    uint8x8_t inMax = vcle_u8(value, max); // "vector comprae less than or equal to"
    return inMin[0] && inMax[0] && inMin[1] && inMax[1] && inMin[2] && inMax[2];
}
#endif

#if BLOB_USE_NEON
uint8x8_t minBallData, maxBallData, minLineData, maxLineData, minBlueData, maxBlueData, minYellowData, maxYellowData;
#else
uint8_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
#endif
static pthread_t threads[BLOB_NUM_THREADS] = {0};
static rpa_queue_t *queues[BLOB_NUM_THREADS] = {0};
static uint8_t *receivedFrame = NULL; // READ ONLY: this is just a pointer to buffer->data in the last MMAL_BUFFER_HEADER_T
static uint8_t *processedBall = NULL; // global processed ball data, allocated each time blob_detector_post is called
static uint8_t *processedGoal = NULL;
static pthread_cond_t doneCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t doneMutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t doneThreads = 0; // number of completed threads

void *blob_worker(void *param){
    int32_t id = (int32_t) param;
    log_trace("Blob worker running with id: %d", id);

    while (true){
        void *queueData = NULL;
        if (!rpa_queue_pop(queues[id], &queueData)){
            log_error("Worker %d: Failed to pop item from queue");
            continue;
        }
        array_range_t *range = (array_range_t*) queueData;
        uint32_t counter = 0;

        for (uint32_t i = range->min; i < range->max; i++){
            uint8_t colour[3] = {receivedFrame[i + R], receivedFrame[i + G], receivedFrame[i + B]};
            bool isBall = in_range(colour, minBallData, maxBallData);
            bool isGoal = in_range(colour, minYellowData, maxYellowData) || in_range(colour, minBlueData, maxBlueData);
            processedBall[i] = isBall ? 255 : 0;
            processedGoal[i] = isGoal ? 255 : 0;

//            if (counter++ % (id + 1) == 0){
//                processedBall[i] = 255;
//            } else {
//                processedBall[i] = 0;
//            }

//            processedBall[i] = id % 2 == 0 ? 255 : 0;

        }

        // notify the main thread we're done
        pthread_mutex_lock(&doneMutex);
        doneThreads++;
        pthread_cond_signal(&doneCond);
        pthread_mutex_unlock(&doneMutex);
        free(range);
    }
}

void blob_detector_init(uint16_t width, uint16_t height){
    log_trace("Initialising blob detector...");
//    receivedFrame = calloc(width * height * 3, sizeof(uint8_t));
    pthread_cond_init(&doneCond, NULL);
    pthread_mutex_init(&doneMutex, NULL);

    log_debug("Creating %d blob detector worker thread(s)", BLOB_NUM_THREADS);
    for (int i = 0; i < BLOB_NUM_THREADS; i++){
        if (!rpa_queue_create(&queues[i], 2)){
            log_error("Failed to create work queue for blob thread %d", i);
        }

        int err = pthread_create(&threads[i], NULL, blob_worker, (void*) i);
        if (err != 0){
            log_error("Failed to create blob worker thread %d: %s", i, strerror(err));
        } else {
            char buf[32];
            sprintf(buf, "Blob Worker %d", i);
            pthread_setname_np(threads[i], buf);
        }
    }
}

uint8_t *blob_detector_post(MMAL_BUFFER_HEADER_T *buffer, uint16_t width, uint16_t height){
    processedBall = malloc(width * height); // would probably make more sense not to free this but RD code gets confusing
    processedGoal = malloc(width * height);
    receivedFrame = buffer->data; // since received frame is read only, we don't need to memcpy
    doneThreads = 0;
    int32_t divisionSize = (width * height) / BLOB_NUM_THREADS;

    // setup work queues (essentially, divide the image into equally sized strips)
    doneThreads = 0;
    uint32_t last = 0;
    for (int i = 0; i < BLOB_NUM_THREADS; i++){
        array_range_t *range = malloc(sizeof(array_range_t));
        range->min = last;
        range->max = last + divisionSize;
        last += divisionSize;
        if (!rpa_queue_trypush(queues[i], range)){
            log_error("Failed to push frame region to blob worker %d (this probably indicates severe performance issues)", i);
        }
    }

    // wait for threads to complete, each time a thread completes it will post the done condition
    pthread_mutex_lock(&doneMutex);
    while (doneThreads < BLOB_NUM_THREADS){
        pthread_cond_wait(&doneCond, &doneMutex);
    }
    pthread_mutex_unlock(&doneMutex);

    // "processedBall" will be freed later, and "receivedFrame" is not allocated
    // FIXME in future, pass on this as well (in some sort of struct I guess) but for now it's not needed
    free(processedGoal);
    return processedBall;
}

void blob_detector_dispose(void){
    log_trace("Disposing blob detector");
    for (int i = 0; i < BLOB_NUM_THREADS; i++){
        pthread_cancel(threads[i]);
        rpa_queue_destroy(queues[i]);
    }
    pthread_cond_destroy(&doneCond);
    pthread_mutex_destroy(&doneMutex);
}

// FIXME: using this macro breaks the curly brace insertion in CLion for some fucking reason
//#if BLOB_USE_NEON
//void blob_detector_parse_thresh(char *threshStr, uint8x8_t *array){
//#else
//void blob_detector_parse_thresh(char *threshStr, uint8_t *array){
//#endif
void blob_detector_parse_thresh(char *threshStr, uint8_t *array){
    char *token;
    char *threshOrig = strdup(threshStr);
    uint8_t i = 0;
#if BLOB_USE_NEON
    uint8_t arr[8] = {0}; // in NEON mode, we have to use the vector functions to copy it out (apparently)
#endif
    token = strtok(threshStr, ",");

    while (token != NULL){
        char *invalid = NULL;
        int32_t number = strtol(token, &invalid, 10);

        if (number > 255){
            log_error("Invalid threshold string \"%s\": token %s > 255 (not in RGB colour range)", threshOrig, token);
        } else if (strlen(invalid) != 0){
            log_error("Invalid threshold string \"%s\": invalid token: \"%s\"", threshOrig, invalid);
        } else {
#if BLOB_USE_NEON
            // put into temp array, to be copied out later
            arr[i++] = (uint8_t) number;
#else
            // put directly into array
            array[i++] = number;
#endif
            if (i > 3){
                log_error("Too many values for key: %s (max: 3)", threshOrig);
                return;
            }
        }
        token = strtok(NULL, ",");
    }
    // log_trace("Successfully parsed threshold key: %s", threshOrig);
#if BLOB_USE_NEON
    // now we can copy our temp array into the 8x8 vector
    *array = vld1_u8(arr);
#endif
    free(threshOrig);
}