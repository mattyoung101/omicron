#include "remote_debug.h"
#include "defines.h"
#include <turbojpeg.h>
#include <log/log.h>
#include <stdlib.h>
#include "DG_dynarr.h"
#include <stdbool.h>
#include <pthread.h>
#include <rpa_queue.h>
#include <sys/socket.h>
#include <lodepng.h>
#include "DG_dynarr.h"

// Manages encoding camera frames to JPG images (with turbo-jpeg) or PNG images (with lodepng) and sending them over a
// TCP socket to the eventual Kotlin remote debugging application

static tjhandle compressor;
static uint16_t width = 0;
static uint16_t height = 0;
static pthread_t frameThread;
static rpa_queue_t *frameQueue = NULL;
static uint32_t frameCounter = 0;

// source for JPEG encoding: https://stackoverflow.com/a/17671012/5007892

/** process a single frame then exit **/
static void *frame_thread(void *param){
    while (true){
        void *frameData = NULL;

        // receive a new frame or block until one is ready
        if (!rpa_queue_pop(frameQueue, &frameData)){
            log_error("Frame queue pop failed");
            continue;
        }

#if !DEBUG_USE_PNG
        uint8_t *compressedImage = NULL;
        unsigned long jpegSize = 0;
        tjCompress2(compressor, frameData, width, 0, height, TJPF_RGB, &compressedImage, &jpegSize, TJSAMP_444, DEBUG_JPEG_QUALITY,
                TJFLAG_FASTDCT);
        log_trace("JPEG encoder done, num bytes: %lu, written to: %s", jpegSize, filename);

        // send over network
        // currently just write to disk for testing
        char *filename = calloc(32, sizeof(char));
        sprintf(filename, "frame_%d.jpg", frameCounter++);
        FILE *out = fopen(filename, "w");
        fwrite(compressedImage, sizeof(uint8_t), jpegSize, out);
        fclose(out);

        tjFree(compressedImage);
        free(frameData);
        free(filename);
#else
        char *filename = calloc(32, sizeof(char));
        sprintf(filename, "frame_%d.png", frameCounter++);

        uint8_t *pngBuffer;
        size_t pngBufferSize;
        lodepng_encode_memory(&pngBuffer, &pngBufferSize, frameData, width, height, LCT_RGB, 8);
        lodepng_save_file(pngBuffer, pngBufferSize, filename); // temporarily write to disk for testing until TCP arrives

        log_trace("PNG encoder done to file: %s", filename);
        free(filename);
        free(frameData);
        free(pngBuffer);
#endif
    }
    return NULL;
}

void remote_debug_init(uint16_t w, uint16_t h){
    // create socket here

    // init JPEG encoder
    compressor = tjInitCompress();
    width = w;
    height = h;

    // init frame queue
    if (!rpa_queue_create(&frameQueue, 4)){
        log_error("Failed to create frame queue");
    }

    // init frame encoder thread
    int err = pthread_create(&frameThread, NULL, frame_thread, NULL);
    if (err != 0){
        log_error("Failed to create frame encoding thread: %s", strerror(err));
    } else {
        pthread_setname_np(frameThread, "RDEncoder");
    }
}


void remote_debug_post_frame(uint8_t *frame){
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
