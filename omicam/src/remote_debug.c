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
#include "DG_dynarr.h"
#include "protobuf/RemoteDebug.pb.h"
#include "nanopb/pb_encode.h"
#include "pb.h"
#include "utils.h"
#include "dyad/dyad.h"

// Manages encoding camera frames to JPG images (with turbo-jpeg) or PNG images (with lodepng) and sending them over a
// TCP socket to the eventual Kotlin remote debugging application
// source for libjpeg-turbo usage: https://stackoverflow.com/a/17671012/5007892

static tjhandle compressor;
static uint16_t width = 0;
static uint16_t height = 0;
static pthread_t frameThread;
static pthread_t tcpThread;
static rpa_queue_t *frameQueue = NULL;
#if DEBUG_WRITE_FRAME_DISK
static uint32_t frameCounter = 0;
#endif
static _Atomic bool connected = false;
static dyad_Stream *server = NULL;
static dyad_Stream *remote = NULL;

/** Used as an easier way to pass two pointers to the thread queue (since it only takes a void*) */
typedef struct {
    uint8_t *camFrame;
    uint8_t *threshFrame;
} frame_entry_t;

/** encodes and sends the image into a buffer **/
static void encode_and_send(uint8_t *camImg, unsigned long camImgSize, uint8_t *threshImg, unsigned long threshImgSize){
    DebugFrame msg = DebugFrame_init_zero;
    size_t bufSize = camImgSize + threshImgSize + 512; // the buffer size is the image sizes + 1KB of extra protobuf data
    uint8_t *buf = malloc(bufSize); // we'll malloc this since we won't ever send the garbage on the end
    pb_ostream_t stream = pb_ostream_from_buffer(buf, bufSize);

    // fill the protocol buffer with data
    memcpy(msg.defaultImage.bytes, camImg, camImgSize);
    memcpy(msg.threshImage.bytes, threshImg, threshImgSize);
    msg.defaultImage.size = camImgSize;
    msg.threshImage.size = threshImgSize;

    // encode to buffer and send it to socket
    if (!pb_encode(&stream, DebugFrame_fields, &msg)){
        log_error("Protobuf encode failed: %s", PB_GET_ERROR(&stream));
    }
    log_trace("Final protobuf stream size: %d bytes", stream.bytes_written);
    if (connected){
//        size_t bytesWritten = stream.bytes_written;
//        dyad_write(server, buf, bufSize);
        dyad_writef(server, strdup("DISP")); // fixme leak
        log_trace("Dispatching frame over TCP");
    }
    free(buf);
}

/**
 * Compresses an image to JPEG with libturbo-jpeg
 * @param frameData the RGB framebuffer
 * @param jpegSize a pointer to a variable holding the size in bytes of the JPEG image
 * @return the encoded JPEG image. Must be freed manually.
 */
static uint8_t *compress_image(uint8_t *frameData, unsigned long *jpegSize){
    uint8_t *compressedImage = NULL;
    tjCompress2(compressor, frameData, width, 0, height, TJPF_RGB, &compressedImage, jpegSize,
                TJSAMP_420, DEBUG_JPEG_QUALITY, TJFLAG_FASTDCT);
    return compressedImage;
}

/** process a single frame then exit **/
static void *frame_thread(GCC_UNUSED void *param){
    while (true){
        void *queueData = NULL;
        if (!rpa_queue_pop(frameQueue, &queueData)){
            log_error("Frame queue pop failed");
            free(queueData); // worst that can happen is this resolves to free(NULL) which is fine
            continue;
        }
        frame_entry_t *entry = (frame_entry_t*) queueData;
        uint8_t *camFrame = entry->camFrame; // normal view from the camera
        uint8_t *threshFrame = entry->threshFrame; // thresholded view from the camera

        unsigned long camImageSize = 0;
        unsigned long threshImageSize = 0;
        uint8_t *camImgCompressed = compress_image(camFrame, &camImageSize);
        uint8_t *threshImgCompressed = compress_image(threshFrame, &threshImageSize);

#if DEBUG_WRITE_FRAME_DISK
        char *filename = calloc(32, sizeof(char));
        sprintf(filename, "frame_%d.jpg", frameCounter++);
        FILE *out = fopen(filename, "w");
        fwrite(camImgCompressed, sizeof(uint8_t), camImageSize, out);
        fclose(out);
        log_trace("JPEG encoder done (size: %lu bytes), written to: %s", camImageSize, filename);
        free(filename);
#else
        log_trace("JPEG encoder done, cam img: %lu bytes, thresh img: %lu bytes, total: %lu bytes", camImageSize,
                  threshImageSize, threshImageSize + camImageSize);
        encode_and_send(camImgCompressed, camImageSize, threshImgCompressed, threshImageSize);
#endif
        tjFree(camImgCompressed); // these two are allocated by tjCompress2 when compress_image is called in this thread
        tjFree(threshImgCompressed);
        free(camFrame); // this is malloc'd and copied from MMAL_BUFFER_HEADER_T in camera_manager
        free(threshFrame); // this is malloc'd and copied from glReadPixels in gpu_manager
        free(entry); // this is malloc'd below and must be freed
    }
    return NULL;
}

static void *dyad_update_thread(GCC_UNUSED void *arg){
    log_trace("TCP thread running");
    while (true) {
        while (dyad_getStreamCount() > 0) {
            dyad_update();
        }
    }
    return NULL;
}

static void dyad_onAccept(dyad_Event *event);

static void dyad_onDisconnect(dyad_Event *event){
    log_info("Debug app client %s:%d timed out or disconnected", dyad_getAddress(event->stream), dyad_getPort(event->stream));
    remote = NULL;
    connected = false;

    // re-open the server for a new connection
    // TODO it does NOT appear that the use after free occurs here, nonetheless this code is shit and should be rewritten
    dyad_close(server);
    server = dyad_newStream();
    dyad_addListener(server, DYAD_EVENT_ACCEPT, dyad_onAccept, NULL);
    dyad_listen(server, DEBUG_PORT);
}

static void dyad_onAccept(dyad_Event *event) {
    log_info("Accepted debug app connection from %s:%d", dyad_getAddress(event->remote), dyad_getPort(event->remote));
    remote = event->remote;
    dyad_addListener(event->remote, DYAD_EVENT_TIMEOUT, dyad_onDisconnect, NULL);
    dyad_addListener(event->remote, DYAD_EVENT_CLOSE, dyad_onDisconnect, NULL);
    connected = true;
}

void remote_debug_init(uint16_t w, uint16_t h){
    log_trace("Initialising remote debugger...");
    compressor = tjInitCompress();
    width = w;
    height = h;
    if (!rpa_queue_create(&frameQueue, 4)){
        log_error("Failed to create frame queue");
    }

    // init frame thread
    int err = pthread_create(&frameThread, NULL, frame_thread, NULL);
    if (err != 0){
        log_error("Failed to create frame encoding thread: %s", strerror(err));
    } else {
        pthread_setname_np(frameThread, "RDEncoder");
    }

    // init TCP socket
    dyad_init();
    server = dyad_newStream();
    dyad_addListener(server, DYAD_EVENT_ACCEPT, dyad_onAccept, NULL);
    dyad_listen(server, DEBUG_PORT);

    err = pthread_create(&tcpThread, NULL, dyad_update_thread, NULL);
    if (err != 0){
        log_error("Failed to create TCP thread: %s", strerror(err));
    } else {
        pthread_setname_np(tcpThread, "TCPThread");
    }

    log_debug("Remote debugger initialised successfully");
}


void remote_debug_post_frame(uint8_t *camFrame, uint8_t *threshFrame){
    frame_entry_t *entry = malloc(sizeof(frame_entry_t));
    entry->camFrame = camFrame;
    entry->threshFrame = threshFrame;

    if (!rpa_queue_trypush(frameQueue, entry)){
        log_warn("Failed to push new frame to queue (perhaps it's full)");
    }
}

void remote_debug_dispose(){
    log_trace("Disposing remote debugger");
    pthread_cancel(frameThread);
    pthread_cancel(tcpThread);
    rpa_queue_destroy(frameQueue);
    tjDestroy(compressor);
    dyad_shutdown();
}