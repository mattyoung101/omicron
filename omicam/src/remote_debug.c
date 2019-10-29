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

typedef struct {
    uint8_t *camFrame;
    uint8_t *threshFrame;
} frame_entry_t;

/** encodes and sends the image into a buffer **/
static void encode_and_send(uint8_t *image, uint32_t imageSize){
    DebugFrame msg = DebugFrame_init_zero;
    size_t bufSize = imageSize + 1024; // the buffer size is the image size + 1KB of extra protobuf data
    uint8_t *buf = calloc(bufSize, sizeof(uint8_t));
    pb_ostream_t stream = pb_ostream_from_buffer(buf, bufSize);

    // fill the protocol buffer with data
    memcpy(msg.defaultImage.bytes, image, imageSize);
    msg.defaultImage.size = imageSize;

    // TODO add support for sending the thresholded image as well

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
            continue;
        }
        frame_entry_t *entry= (frame_entry_t*) queueData;
        uint8_t *camFrame = entry->camFrame;
        uint8_t *threshFrame = entry->threshFrame;

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
//        encode_and_send(camImgCompressed, camImageSize);
#endif
        tjFree(camImgCompressed); // these two are allocated by tjCompress2 when compress_image is called
        tjFree(threshImgCompressed);
        free(camFrame); // these two are allocated elsewhere in the program each tick and must be freed
        free(threshFrame);
        free(entry); // this is malloc'd below and must be freed
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
    zed_net_socket_close(&tcpSocket);
    zed_net_socket_close(&remoteSocket);
    zed_net_shutdown();
}