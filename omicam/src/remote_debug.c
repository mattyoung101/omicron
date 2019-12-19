#include "remote_debug.h"
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
#include "utils.h"
#include "alloc_pool.h"
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <zlib.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "nanopb/pb_decode.h"
#include "protobuf/RemoteDebug.pb.h"
#include <sys/reboot.h>

// Manages encoding camera frames to JPG images (with turbo-jpeg) or PNG images (with lodepng) and sending them over a
// TCP socket to the eventual Kotlin remote debugging application
// source for libjpeg-turbo usage: https://stackoverflow.com/a/17671012/5007892

static tjhandle compressor;
static _Atomic uint16_t width = 0;
static _Atomic uint16_t height = 0;
static pthread_t frameThread, tcpThread, thermalThread;
static rpa_queue_t *frameQueue = NULL;
/** the file descriptor of the server socket **/
static _Atomic int sockfd = -1;
/** the file descriptor of the connection to the client **/
static _Atomic int connfd = -1;
static _Atomic float temperature = 0.0f;
/** true if the device is likely to be thermal throttling **/
static bool thermalThrottling = false;
field_objects_t selectedFieldObject = OBJ_NONE;

static void init_tcp_socket(void);

/** Quickly encodes and sends the given DebugCommand back to Omicontrol as a response **/
static void send_response(DebugCommand command){
    uint8_t buf[512] = {0};
    pb_ostream_t stream = pb_ostream_from_buffer(buf, 512);

    RDMsgFrame wrapper = RDMsgFrame_init_zero;
    wrapper.command = command;
    wrapper.whichMessage = 2;

    if (!pb_encode_delimited(&stream, RDMsgFrame_fields, &wrapper)){
        log_error("Failed to encode Omicontrol response: %s", PB_GET_ERROR(&stream));
    } else {
        ssize_t bytesWritten = write(connfd, buf, stream.bytes_written);
        if (bytesWritten == -1){
            log_warn("Failed to send response to Omicontrol: %s", strerror(errno));
        }
    }
}

/** reads and processes Omicontrol messages. (mostly) non-blocking. **/
static void read_remote_messages(void){
    // read potential protobuf command from Omicontrol socket, recycling buffer
    // https://stackoverflow.com/a/3054519/5007892
    int availableBytes = 0;
    ioctl(connfd, FIONREAD, &availableBytes);

    if (availableBytes > 0) {
        uint8_t *buf = malloc(availableBytes + 1);
        ssize_t readBytes = recv(connfd, buf, availableBytes, MSG_WAITALL);
        if (readBytes == -1){
            log_warn("Failed to read remote message: %s", strerror(readBytes));
            return;
        }
        // printf("recv() return code: %d\n", readBytes);

        pb_istream_t inputStream = pb_istream_from_buffer(buf, 512);
        DebugCommand message = DebugCommand_init_zero;

        if (!pb_decode_delimited(&inputStream, DebugCommand_fields, &message)){
            log_error("Failed to decode Omicontrol Protobuf command: %s", PB_GET_ERROR(&inputStream));
            log_debug("Diagnostic information. readBytes: %d, availableBytes: %d", readBytes, availableBytes);
            free(buf);
            return;
        } else {
            // printf("Successful decode of %d availableBytes, %d readBytes\n", availableBytes, readBytes);
        }

        switch (message.messageId){
            case CMD_THRESHOLDS_GET_ALL: {
                log_debug("Received CMD_THRESHOLD_GET_ALL");

                DebugCommand response = DebugCommand_init_zero;
                response.messageId = CMD_OK;
                int i = 0;
                for (int obj = 1; obj <= 4; obj++) {
                    int32_t *min = thresholds[i++];
                    int32_t *max = thresholds[i++];
                    log_trace("obj id %d, min (%d,%d,%d), max (%d,%d,%d)", obj, min[0], min[1], min[2], max[0], max[1], max[2]);
                    // memcpy apparently doesn't work with this so we have to do it semi-manually
                    for (int j = 0; j < 3; j++) {
                        response.allThresholds[obj].min[j] = min[j];
                    }
                    for (int j = 0; j < 3; j++) {
                        response.allThresholds[obj].max[j] = max[j];
                    }
                }
                send_response(response);
                break;
            }
            case CMD_THRESHOLDS_SELECT: {
                selectedFieldObject = message.objectId;
                log_debug("Received CMD_THRESHOLDS_SELECT, new field object id is %d", selectedFieldObject);
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_THRESHOLDS_SET: {
                log_debug("Received CMD_THRESHOLDS_SET, type=%s, colour channel=%d, value=%d", message.minMax ? "min" : "max",
                        message.colourChannel, message.value);
                // do some magic maths to figure out where in the thresholds array we need to access (NOLINTNEXTLINE)
                int index = (selectedFieldObject - 1) * 2 + (message.minMax ^ 1);
                thresholds[index][message.colourChannel] = message.value;
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_THRESHOLDS_WRITE_DISK: {
                log_debug("Received CMD_THRESHOLDS_WRITE_DISK");
                // TODO you would do it here, check the utils function
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_POWER_OFF: {
                log_info("Received CMD_POWER_OFF");
#if BUILD_TARGET == BUILD_TARGET_PC
                log_warn("Not shutting down as this is a PC build.");
#else
                // https://stackoverflow.com/questions/2678766/how-to-restart-linux-from-inside-a-c-program
                log_info("Shutting down Jetson now...");
                sync();
                reboot(RB_POWER_OFF);
#endif
                break;
            }
            case CMD_POWER_REBOOT: {
                log_info("Received CMD_POWER_REBOOT");
#if BUILD_TARGET == BUILD_TARGET_PC
                log_warn("Not rebooting as this is a PC build.");
#else
                // https://stackoverflow.com/questions/2678766/how-to-restart-linux-from-inside-a-c-program
                log_info("Reeobting Jetson now...");
                sync();
                reboot(RB_AUTOBOOT);
#endif
                break;
            }
            default: {
                log_warn("Unhandled debug message id: %d", message.messageId);
                break;
            }
        }

        free(buf);
    }
}

/** called to handle when Omicontrol disconnects **/
static void client_disconnected(void){
    log_info("Client has disconnected. Restarting socket server...");
    close(sockfd);
    close(connfd);
    connfd = -1;
    init_tcp_socket();
}

/**
 * Encodes the given JPEG images into a Protocl Buffer and disaptches it over the TCP socket, if connected.
 */
static void encode_and_send(uint8_t *camImg, unsigned long camImgSize, uint8_t *threshImg, unsigned long threshImgSize, frame_entry_t *entry){
    DebugFrame msg = DebugFrame_init_zero;
    size_t bufSize = camImgSize + threshImgSize + 1024; // the buffer size is the image sizes + 1KB of extra protobuf data
    uint8_t *buf = malloc(bufSize); // we'll malloc this since we won't ever send the garbage on the end
    pb_ostream_t stream = pb_ostream_from_buffer(buf, bufSize);

    // fill the protocol buffer with data
    memcpy(msg.defaultImage.bytes, camImg, camImgSize);
    memcpy(msg.ballThreshImage.bytes, threshImg, threshImgSize);
    msg.defaultImage.size = camImgSize;
    msg.ballThreshImage.size = threshImgSize;
    msg.temperature = temperature;
    msg.ballCentroid = entry->ballCentroid;
    msg.ballRect = entry->ballRect;

    RDMsgFrame wrapper = RDMsgFrame_init_zero;
    wrapper.frame = msg;
    wrapper.whichMessage = 1;

    if (!pb_encode_delimited(&stream, RDMsgFrame_fields, &wrapper)){
        log_error("Protobuf encode failed: %s", PB_GET_ERROR(&stream));
    }

    // dispatch the buffer over TCP and handle errors
    if (connfd != -1){
        // TODO this seems to hang if the client terminates unexpectedly/refuses to read
        ssize_t written = write(connfd, buf, stream.bytes_written);
        if (written == -1){
            log_warn("Failed to write to TCP socket: %s", strerror(errno));

            if (errno == EPIPE || errno == ECONNRESET){
                client_disconnected();
            }
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
/**
 * The frame thread is the main thread in the remote debugger which waits for a frame to be posted, then encodes
 * and sends it. It also handles receiving commands from Omicontrol.
 */
static void *frame_thread(void *param){
    log_trace("Frame encode thread started");

    while (true){
        void *queueData = NULL;
        // to speed up processing of Omicontrol commands, we wait 5 ms for a frame and if no frame has come
        // in, then quickly process Omicam messages (since this is non blocking) then continue waiting.
        // this way we received a balance between not spamming loops but also processing stuff fast.
        // if there is no connection, then delay indefinitely to save CPU rather than looping
        if (!rpa_queue_timedpop(frameQueue, &queueData, remote_debug_is_connected() ? 5 : RPA_WAIT_FOREVER)){
            read_remote_messages();
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

        read_remote_messages();
        uint8_t *threshImgCompressed = compress_thresh_image(threshFrame, &threshImageSize);
        uint8_t *camImgCompressed = compress_image(camFrame, &camImageSize);
        encode_and_send(camImgCompressed, camImageSize, threshImgCompressed, threshImageSize, entry);

        tjFree(camImgCompressed); // allocated by tjCompress2 when compress_image is called in this thread
        free(threshImgCompressed); // allocated by compress_thresh_frame when zlib compressing
        free(camFrame); // this is malloc'd and copied from the OpenCV frame in computer_vision.cpp
        free(threshFrame); // this is malloc'd and copied from the OpenCV threshold frame in computer_vision.cpp
        free(entry); // this is malloc'd in this file when we push to the queue
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    return NULL;
}

/** reads the CPU temperature every DEBUG_TEMP_REPORTING_INTERVAL seconds **/
static void *thermal_thread(void *arg){
    log_trace("Thermal thread started");

    while (true){
        // FIXME update this to work with jetson
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
        // log_debug("Temperature is %.2f degrees", tempDegrees);

        if (tempDegrees >= 80.0f && !thermalThrottling){
            log_warn("CPU will probably be thermal throttling now (current temp: %.2f degrees, max is 80 degrees)", tempDegrees);
            thermalThrottling = true;
        } else if (tempDegrees <= 75.0f && thermalThrottling){
            log_info("CPU will probably NO LONGER be thermal throttling now (current temp: %.2f degrees)", tempDegrees);
            thermalThrottling = false;
        }

        sleep(DEBUG_TEMP_REPORTING_INTERVAL);
    }
}

/** this thread handles establishing a TCP socket, and waits for a client to connect **/
static void *tcp_thread(void *arg){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr = {0};
    struct sockaddr_in clientAddr = {0};
    socklen_t clientAddrLen;

    if (sockfd == -1){
        log_error("Failed to create TCP socket: %s", strerror(errno));
        return NULL;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(DEBUG_PORT);
    // https://stackoverflow.com/a/24194999/5007892
    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

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
        log_info("Accepted client connection from %s:%d, streaming will now begin", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        // reset selected object to OBJ_NONE to match what Omicontrol will be set to now
        selectedFieldObject = OBJ_NONE;
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
    if (err != 0) {
        log_error("Failed to create thermal reporting thread: %s", strerror(err));
    } else {
        pthread_setname_np(thermalThread, "Thermal Thread");
    }

    init_tcp_socket();
    log_debug("Remote debugger initialised successfully");
}

void remote_debug_post(uint8_t *camFrame, uint8_t *threshFrame, RDRect ballRect, RDPoint ballCentroid){
#if !DEBUG_ALWAYS_SEND
    // we're not connected so free this data
    if (connfd == -1){
        free(camFrame);
        free(threshFrame);
        return;
    }
#endif
    frame_entry_t *entry = malloc(sizeof(frame_entry_t));
    entry->camFrame = camFrame;
    entry->threshFrame = threshFrame;
    entry->ballRect = ballRect;
    entry->ballCentroid = ballCentroid;

    if (!rpa_queue_trypush(frameQueue, entry)){
        log_warn("Failed to push new frame to queue (perhaps it's full or network is busy)");
        free(camFrame);
        free(entry);
        free(threshFrame);
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