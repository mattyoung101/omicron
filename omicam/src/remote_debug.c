#include "remote_debug.h"
#include "defines.h"
#include <turbojpeg.h>
#include <log/log.h>
#include <stdlib.h>
#include "DG_dynarr.h"
#include <stdbool.h>
#include <pthread.h>
#include <rpa_queue.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "DG_dynarr.h"
#include "protobuf/RemoteDebug.pb.h"
#include "nanopb/pb_encode.h"
#include "pb.h"
#include "utils.h"
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <zlib.h>
#include <arpa/inet.h>

// Manages encoding camera frames to JPG images (with turbo-jpeg) or PNG images (with lodepng) and sending them over a
// TCP socket to the eventual Kotlin remote debugging application
// source for libjpeg-turbo usage: https://stackoverflow.com/a/17671012/5007892

static tjhandle compressor;
static _Atomic uint16_t width = 0;
static _Atomic uint16_t height = 0;
static pthread_t frameThread, tcpThread, thermalThread;
static rpa_queue_t *frameQueue = NULL;
static _Atomic int sockfd, connfd = -1;
static _Atomic float temperature = 0.0f;
static bool thermalThrottling = false;

/** Used as an easier way to pass two pointers to the thread queue (since it only takes a void*) */
typedef struct {
    uint8_t *camFrame;
    uint8_t *threshFrame;
} frame_entry_t;

static void init_tcp_socket(void);

/**
 * Encodes the given JPEG images into a Protocl Buffer and disaptches it over the TCP socket, if connected.
 */
static void encode_and_send(uint8_t *camImg, unsigned long camImgSize, uint8_t *threshImg, unsigned long threshImgSize){
    DebugFrame msg = DebugFrame_init_zero;
    size_t bufSize = camImgSize + threshImgSize + 512; // the buffer size is the image sizes + 1KB of extra protobuf data
    uint8_t *buf = malloc(bufSize); // we'll malloc this since we won't ever send the garbage on the end
    pb_ostream_t stream = pb_ostream_from_buffer(buf, bufSize);

    // fill the protocol buffer with data
    memcpy(msg.defaultImage.bytes, camImg, camImgSize);
    memcpy(msg.ballThreshImage.bytes, threshImg, threshImgSize);
    msg.defaultImage.size = camImgSize;
    msg.ballThreshImage.size = threshImgSize;
    msg.temperature = temperature;

    // encode to buffer
    if (!pb_encode_delimited(&stream, DebugFrame_fields, &msg)){
        log_error("Protobuf encode failed: %s", PB_GET_ERROR(&stream));
    }

    // dispatch the buffer over TCP and handle errors
    if (connfd != -1){
        // TODO this seems to hang if the client terminates unexpectedly/refuses to read
        ssize_t written = write(connfd, buf, stream.bytes_written);
        if (written == -1){
            log_warn("Failed to write to TCP socket: %s", strerror(errno));

            if (errno == EPIPE || errno == ECONNRESET){
                log_info("Assuming client has disconnected, restarting socket server");
                close(sockfd);
                close(connfd);
                connfd = -1;
                init_tcp_socket();
            }
        } else {
//            log_trace("Written %d bytes successfully to TCP", written);
        }
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

/**
 * Compresses the given buffer, from the blob detector, with zlib. Assumes the buffer size is equal to width * height.
 * @param inBuffer the output of blob_detector_post(), must be width * height bytes
 * @return a newly allocated buffer with the compressed data, must be freed
 */
static uint8_t *compress_thresh_image(uint8_t *inBuffer, unsigned long *outputSize){
    unsigned long inSize = width * height;
    unsigned long outSize = compressBound(width * height);
    uint8_t *outBuf = malloc(outSize);

    int result = compress2(outBuf, &outSize, inBuffer, inSize, DEBUG_COMPRESSION_LEVEL);
    if (result != Z_OK){
        log_error("Failed to zlib compress buffer, error: %d", result);
        return NULL;
    } else {
        *outputSize = outSize;
        return outBuf;
    }
}

// TODO set frame thread priority!
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

        // if the application is shutdown now, we don't want to cancel this thread until we've safely allocated and
        // de-allocated everything (to prevent memory leaks and stop ASan from complaining)
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        uint8_t *threshImgCompressed = compress_thresh_image(threshFrame, &threshImageSize);
        uint8_t *camImgCompressed = compress_image(camFrame, &camImageSize);
        encode_and_send(camImgCompressed, camImageSize, threshImgCompressed, threshImageSize);

        tjFree(camImgCompressed); // allocated by tjCompress2 when compress_image is called in this thread
        tjFree(threshImgCompressed); // allocated by compress_thresh_frame when zlib compressing
        free(camFrame); // this is malloc'd and copied from MMAL_BUFFER_HEADER_T in camera_manager
        free(threshFrame); // this is malloc'd by the blob detector
        free(entry); // this is malloc'd when we push to the queue and must be freed
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    return NULL;
}

/** reads the CPU temperature every DEBUG_TEMP_REPORTING_INTERVAL seconds **/
static void *thermal_thread(void *arg){
    while (true){
        // general idea from https://www.raspberrypi.org/forums/viewtopic.php?t=170112#p1091895
        FILE *tempFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
        char buf[32];
        if (tempFile == NULL){
            log_error("Failed to open temperature source, aborting thermal monitor.");
            return NULL;
        }
        if (fgets(buf, 32, tempFile) == NULL){
            log_warn("Failed to read temperature");
            continue;
        }
        fclose(tempFile);

        long tempLong = strtol(buf, NULL, 10);
        float tempDegrees = (float) tempLong / 1000.0f;
        temperature = tempDegrees;
        log_debug("Temperature is %f degrees", tempDegrees);

        if (temperature >= 80.0f && !thermalThrottling){
            log_warn("The temperature is currently %.2f degrees, which means the ARM CPU will enable thermal throttling to prevent damage.",
                    temperature);
            log_warn("Please review your cooling setup, and install/upgrade your cooler if deemed necessary.");
            thermalThrottling = true;
        } else if (temperature < 75.0f && thermalThrottling){
            log_info("The temperature has returned to normal, so the ARM CPU will disable thermal throttling (currently: %.2f degrees)",
                    temperature);
            thermalThrottling = false;
        }

        sleep(DEBUG_TEMP_REPORTING_INTERVAL);
    }
}

static void *tcp_thread(void *arg){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr = {0};
    struct sockaddr_in clientAddr = {0};
    socklen_t clientAddrLen = 69;

    if (sockfd == -1){
        log_error("Failed to create TCP socket: %s", strerror(errno));
        return NULL;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(DEBUG_PORT);

    if (bind(sockfd, &serverAddr, sizeof(serverAddr)) != 0){
        log_error("Failed to bind TCP socket: %s", strerror(errno));
        return NULL;
    }
    if (listen(sockfd, 4) != 0){
        log_error("Failed to listen TCP socket: %s", strerror(errno));
        return NULL;
    }
    clientAddrLen = sizeof(clientAddr);
    log_debug("Awaiting client connection to TCP socket");

    // block until a connection occurs (hence why this runs on its own thread)
    connfd = accept(sockfd, (struct sockaddr*) &clientAddr, &clientAddrLen);
    if (connfd == -1){
        log_error("Failed to accept TCP connection: %s", strerror(errno));
        close(sockfd);
        return NULL;
    } else {
        log_info("Accepted client connection from %s:%d", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    }
    return NULL;
}

static void init_tcp_socket(void){
    int err = pthread_create(&tcpThread, NULL, tcp_thread, NULL);
    if (err != 0){
        log_error("Failed to create TCP thread: %s", strerror(err));
    } else {
        pthread_setname_np(tcpThread, "TCP Thread");
    }
}

void remote_debug_init(uint16_t w, uint16_t h){
    log_trace("Initialising remote debugger...");
    compressor = tjInitCompress();
    width = w;
    height = h;
    if (!rpa_queue_create(&frameQueue, 4)){
        log_error("Failed to create frame queue");
    }

    // init threads
    int err = pthread_create(&frameThread, NULL, frame_thread, NULL);
    if (err != 0){
        log_error("Failed to create frame encoding thread: %s", strerror(err));
    } else {
        pthread_setname_np(frameThread, "RD Encoder");
    }
    err = pthread_create(&thermalThread, NULL, thermal_thread, NULL);
    if (err != 0){
        log_error("Failed to create thermal reporting thread: %s", strerror(err));
    } else {
        pthread_setname_np(thermalThread, "ThermalMonitor");
    }

    init_tcp_socket();
    log_debug("Remote debugger initialised successfully");
}

void remote_debug_post_frame(uint8_t *camFrame, uint8_t *threshFrame){
    if (connfd == -1){
        free(camFrame);
        free(threshFrame);
        return;
    }
    frame_entry_t *entry = malloc(sizeof(frame_entry_t));
    entry->camFrame = camFrame;
    entry->threshFrame = threshFrame;

    if (!rpa_queue_trypush(frameQueue, entry)){
        log_warn("Failed to push new frame to queue (perhaps it's full or network is busy)");
    }
}

void remote_debug_dispose(void){
    log_trace("Disposing remote debugger");
    // FIXME if you are connected to remote when this is called, you get a segfault in libturbojpeg memcpy somewhere (see ASan output)
    pthread_cancel(frameThread);
    pthread_cancel(tcpThread);
    pthread_cancel(thermalThread);
    rpa_queue_destroy(frameQueue);
    tjDestroy(compressor);
    close(sockfd);
    close(connfd);
    connfd = -1;
}

bool remote_debug_is_connected(void){
#if DEBUG_ALWAYS_SEND
    return true;
#else
    return connfd != -1;
#endif
}