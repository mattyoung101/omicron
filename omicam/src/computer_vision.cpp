#include "computer_vision.hpp"
#include <opencv2/core/utility.hpp>
#include "log/log.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/mat.hpp"
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaobjdetect.hpp>
#include <map>
#include <unistd.h>
#include "defines.h"
#include "utils.h"
#include "remote_debug.h"
#include "cuda/in_range.cuh"
#include "rpa_queue.h"
// yeah this is bad practice, what are you gonna do?
using namespace cv;
using namespace std;

// This file handles all the OpenCV tasks in C++ code, which is then called by the C code in the rest of the project,
// as OpenCV dropped support for the legacy C API.
// I also sincerely apologise in advance for the somewhat hideous mix of C-style code and modern C++-style code, I didn't have
// the time or energy to learn proper modern C++ for just this file in a majority C project.

/** A detected field object and its associated information */
typedef struct {
    bool exists;
    RDPoint centroid;
    RDRect boundingBox;
    Mat threshMask;
} object_result_t;

/** An entry in a vision thread work queue */
typedef struct {
    int32_t *min;
    int32_t *max;
    field_objects_t objectId;
} vision_entry_t;

static pthread_t cvThread = {0};
static pthread_t fpsCounterThread = {0};
static _Atomic int32_t fpsCounter = 0;
static _Atomic int32_t lastFpsMeasurement = 0;
static pthread_t workThreads[4] = {0};
static rpa_queue_t *workQueues[4] = {0}; // a queue that each worker has to receive new items to process from
static cuda::GpuMat *currentFrame, *currentResizedFrame; // the current frame, converted to RGB on the GPU, that will be processed by the workers
static object_result_t results[4] = {0}; // results of the vision processing goes into this array

/**
 * Uses the specified threshold values to threshold the given frame and detect the specified object on the field.
 * This is in a separate function so that we can just invoke it for each object, likes "ball", "lines", etc.
 * TODO make frame a GpuMat
 * @param frame the GPU accelerated Mat containing the current frame being processed
 * @param min an array containing the 3 minimum RGB values
 * @param max an array containing the 3 maximum RGB values
 * @param objectId what the object is
 */
static object_result_t process_object(cuda::GpuMat frame, int32_t *min, int32_t *max, field_objects_t objectId){
    cuda::Stream queue;
    cuda::GpuMat thresholdedGPU(frame.rows, frame.cols, CV_8UC1);
    thresholdedGPU.create(frame.rows, frame.cols, CV_8UC1);

    Mat thresholded, labels, stats, centroids;
    Scalar minScalar = Scalar(min[0], min[1], min[2]);
    Scalar maxScalar = Scalar(max[0], max[1], max[2]);

    // run computer vision tasks
    inRange_gpu(frame, minScalar, maxScalar, thresholdedGPU, queue);
    thresholdedGPU.download(thresholded, queue);
    int nLabels = connectedComponentsWithStats(thresholded, labels, stats, centroids);

    // find the biggest blob, skipping id 0 which is the background
    vector<int> sortedLabels;
    for (int label = 1; label < nLabels; label++){
        sortedLabels.push_back(label);
    }
    sort(sortedLabels.begin(), sortedLabels.end(), [=](const int &a, const int &b){
        int aSize = stats.at<int>(a, CC_STAT_AREA);
        int bSize = stats.at<int>(b, CC_STAT_AREA);
        return aSize < bSize;
    });

    auto blobExists = !sortedLabels.empty();
    auto largestId = !blobExists ? -1 : sortedLabels.back();
    auto largestCentroid = !blobExists ? Point(0, 0) : Point(centroids.at<double>(largestId, 0),
                                                             centroids.at<double>(largestId, 1));
    RDRect objRect = {0};
    if (blobExists) {
        int rectX = stats.at<int>(largestId, CC_STAT_LEFT);
        int rectY = stats.at<int>(largestId, CC_STAT_TOP);
        int rectW = stats.at<int>(largestId, CC_STAT_WIDTH);
        int rectH = stats.at<int>(largestId, CC_STAT_HEIGHT);
        objRect = {rectX, rectY, rectW, rectH};
    }

    object_result_t result = {0}; // NOLINT
    result.exists = blobExists;
    result.centroid = {largestCentroid.x, largestCentroid.y};
    result.boundingBox = objRect;
    result.threshMask = thresholded;
    return result;
}

/** work thread for GPU processing code **/
static auto gpu_work_thread(void *arg) -> void * {
    uint32_t id = *(uint32_t*) arg;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    log_trace("GPU work thread #%d started", id);

    while (true){
        void *queueData = nullptr;
        if (!rpa_queue_pop(workQueues[id], &queueData)){
            log_warn("Worker %d failed to pop item from work queue", id);
            break;
        }

        // printf("worker %d received some data\n", id);
        auto *entry = (vision_entry_t*) queueData;
        free(entry);
    }
    return nullptr;
}

static inline void schedule_worker(int32_t id, int32_t *min, int32_t *max, field_objects_t objectId){
    auto *entry = (vision_entry_t*) malloc(sizeof(vision_entry_t));
    entry->objectId = objectId;
    entry->min = min;
    entry->max = max;
    if (!rpa_queue_trypush(workQueues[id], entry)){
        log_warn("Failed to push item to GPU worker queue. This may indicate performance problems or a hang.");
        free(entry);
    }
}

/** this task runs all the computer vision for Omicam, using OpenCV */
static auto cv_thread(void *arg) -> void *{
    uint32_t rdFrameCounter = 0;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    log_trace("Vision thread started");

#if BUILD_TARGET == BUILD_TARGET_PC
    log_trace("Build target is PC, using test data");
    VideoCapture cap("../orange_ball.m4v");
    if (!cap.isOpened()) {
        log_error("Failed to load OpenCV test video");
    }
    double fps = cap.get(CAP_PROP_FPS);
    log_debug("Video file FPS: %f", fps);
#else
    log_trace("Build target is Jetson, initialising VideoCapture");
    // TODO ... gstreamer and shit ...
    log_trace("OpenCV capture initialised successfully");
#endif

    while (true){
        Mat frame, frameRGB;
        cuda::GpuMat gpuFrame, gpuFrameRGB, gpuFrameScaled;

        cap.read(frame);
        if (frame.empty()){
            log_warn("Received empty frame from capture!");
#if BUILD_TARGET == BUILD_TARGET_PC
            cap.set(CAP_PROP_FRAME_COUNT, 0);
            cap.set(CAP_PROP_POS_FRAMES, 0);
            cap.set(CAP_PROP_POS_AVI_RATIO, 0);
#endif
            continue;
        }

        // do image pre-processing such as resizing, colour conversions etc
        gpuFrame.upload(frame);
        cuda::cvtColor(gpuFrame, gpuFrameRGB, COLOR_BGR2RGB);
        cuda::resize(gpuFrameRGB, gpuFrameScaled, Size(0, 0), VISION_SCALE_FACTOR, VISION_SCALE_FACTOR,
                INTER_NEAREST);

        // prepare workers for new entry
        memset(results, 0, sizeof(object_result_t) * 4);
        currentFrame = &gpuFrameRGB;
        currentResizedFrame = &gpuFrameScaled;

        // process all our field objects
        auto ball = process_object(gpuFrameRGB, minBallData, maxBallData, OBJ_BALL);
        // TODO use scaled frame for yellow and blue goal (will require scaling coords and stuff)
        // auto yellowGoal = process_object(frameRGB, minYellowData, maxYellowData, OBJ_GOAL_YELLOW);
        // auto blueGoal = process_object(frameRGB, minBlueData, maxBlueData, OBJ_GOAL_BLUE);
        auto lines = process_object(gpuFrameRGB, minLineData, maxLineData, OBJ_LINES);

        // schedule_worker(0, minBallData, maxBallData, OBJ_BALL);
        // schedule_worker(1, minLineData, maxLineData, OBJ_LINES);

        // dispatch frames to remote debugger
#if DEBUG_ENABLED
        if (rdFrameCounter++ % DEBUG_FRAME_EVERY == 0 && remote_debug_is_connected()) {
            // download frame off GPU
            gpuFrameRGB.download(frameRGB);

            char buf[128] = {0};
            snprintf(buf, 128, "Frame %d (Omicam v%s), selected object: %s", rdFrameCounter, OMICAM_VERSION,
                     fieldObjToString[selectedFieldObject]);
            putText(frameRGB, buf, Point(10, 25), FONT_HERSHEY_DUPLEX, 0.5,
                    Scalar(255, 0, 0), 1, FILLED, false);

            // copy the frame off the Mat into some raw data that can be processed by the remote debugger code
            // frame is a 3 channel RGB image (hence the times 3)
            auto *frameData = (uint8_t*) malloc(frame.rows * frame.cols * 3);
            memcpy(frameData, frameRGB.data, frame.rows * frame.cols * 3);

            // ballThresh is just a 1-bit mask so it has only one channel
            auto *threshData = (uint8_t*) calloc(ball.threshMask.rows * ball.threshMask.cols, sizeof(uint8_t));
            switch (selectedFieldObject){
                case OBJ_NONE: {
                    // just send the empty buffer
                    remote_debug_post(frameData, threshData, {0, 0, 0, 0}, {0, 0}, lastFpsMeasurement);
                    break;
                }
                case OBJ_BALL: {
                    memcpy(threshData, ball.threshMask.data, ball.threshMask.rows * ball.threshMask.cols);
                    remote_debug_post(frameData, threshData, ball.boundingBox, ball.centroid, lastFpsMeasurement);
                    break;
                }
                case OBJ_LINES: {
                    memcpy(threshData, lines.threshMask.data, lines.threshMask.rows * lines.threshMask.cols);
                    remote_debug_post(frameData, threshData, {0, 0, 0, 0}, {0, 0}, lastFpsMeasurement);
                    break;
                }
                default: {
                    log_warn("Unsupported field object selected: %d", selectedFieldObject);
                    break;
                }
            }

        }
#endif
        // encode protocol buffer to send to ESP over UART
        ObjectData data = ObjectData_init_zero;
        data.ballExists = ball.exists;
        data.ballX = ball.centroid.x;
        data.ballY = ball.centroid.y;
        utils_cv_transmit_data(data);

        fpsCounter++;

#if BUILD_TARGET == BUILD_TARGET_PC
        waitKey(static_cast<int>(1000 / fps));
#endif
    }
    destroyAllWindows();
}

/** monitors the FPS of the vision thread **/
static auto fps_counter_thread(void *arg) -> void *{
    while (true){
        sleep(1);
        lastFpsMeasurement = fpsCounter;
        fpsCounter = 0;
        if (!remote_debug_is_connected()){
            printf("Vision FPS: %d\n", lastFpsMeasurement);
            fflush(stdout);
        }
    }
}

void vision_init(void){
    int numCpus = getNumberOfCPUs();
    bool openCLDevice = false;
    ocl::setUseOpenCL(true);
    ocl::Context ctx = ocl::Context::getDefault();
    if (ctx.ptr())
        openCLDevice = true;
    int numCudaDevices = cuda::getCudaEnabledDeviceCount();
    log_info("OpenCV version: %d.%d.%d", CV_VERSION_MAJOR, CV_VERSION_MINOR, CV_VERSION_REVISION);
    log_info("System info: %d CPU core(s), %d CUDA device(s) available, %s", numCpus, numCudaDevices,
            openCLDevice ? "OpenCL available" : "OpenCL unavailable");
    if (numCudaDevices > 0){
        cuda::printShortCudaDeviceInfo(cuda::getDevice());
    }

    // create work queues
    for (auto & workQueue : workQueues){
        rpa_queue_create(&workQueue, 2);
    }

    // create threads
    int err = pthread_create(&cvThread, nullptr, cv_thread, nullptr);
    if (err != 0){
        log_error("Failed to create OpenCV thread: %s", strerror(err));
    } else {
        pthread_setname_np(cvThread, "Vision Thread");
    }

    err = pthread_create(&fpsCounterThread, nullptr, fps_counter_thread, nullptr);
    if (err != 0){
        log_error("Failed to create FPS counter thread: %s", strerror(err));
    } else {
        pthread_setname_np(fpsCounterThread, "FPS Counter");
    }

    for (int i = 0; i < 4; i++){
        auto *arg = new uint32_t;
        *arg = i;
        pthread_create(&workThreads[i], nullptr, gpu_work_thread, (void*) arg);
    }

    log_info("Vision started");
    pthread_join(cvThread, nullptr);
    // this will block forever, or until the vision thread terminates for some reason
}

void vision_dispose(void){
    log_trace("Stopping vision thread");
    // important note: for some fucking reason you have to cancel these shits first, or the app won't exit properly
    for (auto & thread : workThreads){
        pthread_cancel(thread);
    }
    pthread_cancel(cvThread);
    for (auto & workQueue : workQueues){
        rpa_queue_destroy(workQueue);
    }
}

// and now...
// This is your resident "FUCK YOU" to NVIDIA who couldn't be arsed to implement OpenCL on Tegra chips such as the Jetson Nano.
// Was it really that difficult guys? Now I'm stuck with your shitty, proprietary, non-cross-platform CUDA bullshit that no-one wants.
// Seriously, your fuckin terrible-ass Linux desktop driver even supports it, is it really that difficult to put on a Tegra?
// Also, hardly unexpectedly, the bloody CUDA APIs in OpenCV absolutely suck shit compared to the OpenCL T-API ughhh