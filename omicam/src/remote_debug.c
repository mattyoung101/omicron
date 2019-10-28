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
#include "protobuf/RemoteDebug.pb.h"
#include "nanopb/pb_encode.h"
#include "pb.h"
#include "nanopb/pb_common.h"
#include "utils.h"
#define ZED_NET_IMPLEMENTATION
#include "zed_net.h"

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
static zed_net_socket_t tcpSocket;
static zed_net_socket_t remoteSocket;
static zed_net_address_t remoteAddress;
static _Atomic bool connected = false;
static char *displayCommand[] = {"D", "I", "S", "P"}; // sent to tell the client that a full frame has been sent

/** encodes and sends the image into a buffer **/
static void encode_and_send(frame_t camFrame, frame_t threshFrame){
    DebugFrame msg = DebugFrame_init_zero;
    size_t bufSize = camFrame.size + threshFrame.size + 1024;
    uint8_t *buf = malloc(bufSize);
    pb_ostream_t stream = pb_ostream_from_buffer(buf, bufSize);

    // fill the protocol buffer with data
    memcpy(msg.defaultImage.bytes, camFrame.buf, camFrame.size);
    memcpy(msg.threshImage.bytes, threshFrame.buf, threshFrame.size);
    msg.defaultImage.size = camFrame.size;
    msg.threshImage.size = threshFrame.size;

    // encode to buffer and send it to socket
    if (!pb_encode(&stream, DebugFrame_fields, &msg)){
        log_error("Protobuf encode failed: %s", PB_GET_ERROR(&stream));
    }
    log_trace("Final protobuf stream size: %d bytes", stream.bytes_written);
    if (connected){
        zed_net_tcp_socket_send(&tcpSocket, buf, stream.bytes_written);
        zed_net_tcp_socket_send(&tcpSocket, displayCommand, 4);
        log_trace("Dispatching frame over TCP");
    }
    free(buf);
}

/**
 * Compresses an image to a JPEG or PNG depending on what's defined
 * @param data the image data
 * @return the compressed image buffer, must be freed manually
 */
static frame_t compress_image(uint8_t *data){
    uint8_t *compressedImage = NULL;
    unsigned long jpegSize = 0;
    tjCompress2(compressor, data, width, 0, height, TJPF_RGB, &compressedImage, &jpegSize, TJSAMP_420,
                DEBUG_JPEG_QUALITY, TJFLAG_FASTDCT);
#if DEBUG_WRITE_FRAME_DISK
        char *filename = calloc(32, sizeof(char));
        sprintf(filename, "frame_%d.jpg", frameCounter++);
        FILE *out = fopen(filename, "w");
        fwrite(compressedImage, sizeof(uint8_t), jpegSize, out);
        fclose(out);
        log_trace("JPEG encoder done (size: %lu bytes), written to: %s", jpegSize, filename);
        free(filename);
#else
    log_trace("JPEG encoder done: %lu bytes", jpegSize);
#endif

    frame_t image = {0};
    image.buf = compressedImage;
    image.size = jpegSize;
    return image;
}

/** process a single frame then exit **/
static void *frame_thread(GCC_UNUSED void *param){
    while (true){
        frame_entry_t *entry = NULL;
        if (!rpa_queue_pop(frameQueue, (void*) &entry)){
            log_error("Frame queue pop failed");
            continue;
        }
        // Figured out the cause of the segfault: for whatever reason, tjCompress2 hates empty calloc'd arrays
        // So it seems glReadPixels is not reading anything or is at least reading blank pixels

        // FIXME there is a severe memory corruption bug around here, currently working on fixing it

        puts("about to encode");
        frame_t encodedCamFrame = compress_image(entry->cameraFrame);
        puts("encoded successfully");
//        frame_t encodedThreshFrame = compress_image(entry->threshFrame);
//        encode_and_send(encodedCamFrame, encodedThreshFrame);

        tjFree(encodedCamFrame.buf);
//        tjFree(encodedThreshFrame.buf);
        printf("pointer to camera frame: 0x%X\n", entry->cameraFrame);
        free(entry->cameraFrame);
        free(entry->threshFrame);
        free(entry);
    }
    return NULL;
}

/** thread for waiting for TCP connection **/
static void *tcp_thread(GCC_UNUSED void *arg){
    log_debug("Waiting for incoming TCP connection...");
    if (zed_net_tcp_accept(&tcpSocket, &remoteSocket, &remoteAddress) != 0){
        log_warn("Failed to accept TCP client: %s", zed_net_get_error());
    }
    connected = true;
    const char *host = zed_net_host_to_str(remoteAddress.host);
    log_info("Accepted remote debug connection from %s:%d", host, remoteAddress.port);
    pthread_exit(NULL);
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
    zed_net_init();
    if (zed_net_tcp_socket_open(&tcpSocket, DEBUG_PORT, false, true) != 0){
        log_error("Failed to create TCP socket: %s", zed_net_get_error());
    }

    err = pthread_create(&tcpThread, NULL, tcp_thread, NULL);
    if (err != 0){
        log_error("Failed to create TCP thread: %s", strerror(err));
    } else {
        pthread_setname_np(tcpThread, "TCPThread");
    }
    log_debug("Remote debugger initialised successfully");
}

void remote_debug_post_frame(uint8_t *cameraFrame, uint8_t *threshFrame){
    // both entry, cameraFrame and threshFrame will be free'd by the debug post thread
    frame_entry_t *entry = calloc(1, sizeof(frame_entry_t));
    entry->cameraFrame = cameraFrame;
    entry->threshFrame = threshFrame;

    if (!rpa_queue_trypush(frameQueue, &entry)){
        log_warn("Failed to push new frame to queue (perhaps it's full)");
        free(cameraFrame);
        free(threshFrame);
        free(entry);
    }
}

void remote_debug_dispose(){
    log_trace("Disposing remote debugger");
    pthread_cancel(frameThread);
    pthread_cancel(tcpThread);
    rpa_queue_destroy(frameQueue);
    tjDestroy(compressor);
    zed_net_socket_close(&tcpSocket);
    zed_net_socket_close(&remoteSocket);
    zed_net_shutdown();
}
