#include "remote_debug.h"
#include "defines.h"
#include <turbojpeg.h>
#include <log/log.h>
#include <stdlib.h>

// Manages encoding camera frames to JPG images (with turbo-jpeg) and sending them over a TCP socket to the eventual
// Kotlin remote debugging application

static tjhandle compressor;
static uint8_t *compressBuffer;
static uint16_t width = 0;
static uint16_t height = 0;

void remote_debug_init(uint16_t w, uint16_t h){
    // create socket here

    compressor = tjInitCompress();
    compressBuffer = calloc(w * h * 3, sizeof(uint8_t));
    width = w;
    height = h;
}

// source for encoding: https://stackoverflow.com/a/17671012/5007892
void remote_debug_post_frame(uint8_t *frame, size_t frameSize){
    uint8_t *compressedImage = NULL;
    unsigned long jpegSize = 0;

    tjCompress2(compressor, compressBuffer, width, 0, height, TJPF_RGB, &compressedImage, &jpegSize,
            TJSAMP_444, DEBUG_JPEG_QUALITY, TJFLAG_FASTDCT);
    // send over network

    tjFree(compressedImage);
}


void remote_debug_dispose(){
    log_trace("Disposing remote debugger");
    tjDestroy(compressor);
    free(compressBuffer);
    compressBuffer = NULL;
}


