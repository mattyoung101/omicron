#include "remote_debug.h"
#include "defines.h"
#include <turbojpeg.h>
#include <log/log.h>
#include <stdlib.h>
#include "DG_dynarr.h"
#include <stdbool.h>
#include <pthread.h>
#include <rpa_queue.h>
#include "DG_dynarr.h"

// Manages encoding camera frames to JPG images (with turbo-jpeg) and sending them over a TCP socket to the eventual
// Kotlin remote debugging application

static tjhandle compressor;
static uint16_t width = 0;
static uint16_t height = 0;
static pthread_t frameThread;
static rpa_queue_t *frameQueue = NULL;

// source for JPEG encoding: https://stackoverflow.com/a/17671012/5007892
uint32_t frameCounter = 0;
/** process a single frame then exit **/
static void *frame_thread(void *param){
    while (true){
        void *frameData = NULL;
        uint8_t *frame = NULL;
        uint8_t *compressedImage = NULL;
        unsigned long jpegSize = 0;

        // receive a new frame or block until one is ready
        if (!rpa_queue_pop(frameQueue, &frameData)){
            log_error("Frame queue pop failed");
            free(frameData);
            return NULL;
        }

        // TODO we should add a PNG decoder for frame dumping (to verify quality)

        frame = (uint8_t*) frameData;
        tjCompress2(compressor, frame, width, 0, height, TJPF_RGB, &compressedImage, &jpegSize,
                TJSAMP_444, DEBUG_JPEG_QUALITY, TJFLAG_FASTDCT);

        // send over network (or maybe write to disk)
        char *filename = calloc(32, sizeof(char));
        sprintf(filename, "frame_%d.jpg", frameCounter++);
        FILE *out = fopen(filename, "w");
        fwrite(compressedImage, sizeof(uint8_t), jpegSize, out);
        fclose(out);

        printf("jpeg encoder done, num bytes: %lu, written to: %s\n", jpegSize, filename);

        tjFree(compressedImage);
        free(frameData);
        free(filename);
    }
}

void remote_debug_init(uint16_t w, uint16_t h){
    // create socket here

    compressor = tjInitCompress();
    width = w;
    height = h;

    // create frame decode thread
    if (!rpa_queue_create(&frameQueue, 4)){
        log_error("Failed to create frame queue");
    }

    int err = pthread_create(&frameThread, NULL, frame_thread, NULL);
    if (err != 0){
        log_error("Failed to create frame encoding thread: %s", strerror(err));
    } else {
        pthread_setname_np(frameThread, "RDEncoder");
    }
}


void remote_debug_post_frame(uint8_t *frame, size_t frameSize){
    if (!rpa_queue_trypush(frameQueue, frame)){
        log_warn("Failed to push new frame to queue (perhaps it's full)");
    }
}

void remote_debug_dispose(){
    log_trace("Disposing remote debugger");
    pthread_cancel(frameThread);
    rpa_queue_destroy(frameQueue);
    tjDestroy(compressor);
}


