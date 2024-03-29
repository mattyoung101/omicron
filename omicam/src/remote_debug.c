/*
 * This file is part of the Omicam project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
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
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <zlib.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "nanopb/pb_decode.h"
#include "protobuf/RemoteDebug.pb.h"
#include "localisation.h"
#include "comms_uart.h"
#include <sys/reboot.h>
#include <sys/param.h>
#include <inttypes.h>

// Manages encoding camera frames to JPG images (with turbo-jpeg) and sending them over a TCP socket to Omicontrol
// Also handles receiving data from Omicontrol
// source for libjpeg-turbo usage: https://stackoverflow.com/a/17671012/5007892

static tjhandle compressor;
/** frame width */
static _Atomic int32_t width = 0;
/** frame height */
static _Atomic int32_t height = 0;
static pthread_t frameThread, tcpThread, thermalThread;
static rpa_queue_t *frameQueue = NULL;
/** the file descriptor of the server socket */
static _Atomic int sockfd = -1;
/** the file descriptor of the connection to the client */
static _Atomic int connfd = -1;
_Atomic double cpuTemperature = 0.0f;
/** true if the device is likely to be thermal throttling */
static bool thermalThrottling = false;
field_objects_t selectedFieldObject = OBJ_NONE;
/** the total number of times we've had connection problems with this client */
static int32_t totalFailures = 0;
/** true if we have already acknowledged a connection error occuring */
static bool wasError = false;

static void init_tcp_socket(void);

/** Quickly encodes and sends the given DebugCommand back to Omicontrol as a response **/
static void send_response(DebugCommand command){
    uint8_t buf[1024] = {0};
    pb_ostream_t stream = pb_ostream_from_buffer(buf, 1024);

    RDMsgFrame wrapper = RDMsgFrame_init_zero;
    wrapper.command = command;
    wrapper.whichMessage = 2;

    if (!pb_encode_delimited(&stream, RDMsgFrame_fields, &wrapper)){
        log_error("Failed to encode Omicontrol response: %s (max stream size: %d)", PB_GET_ERROR(&stream), stream.max_size);
    } else {
        ssize_t bytesWritten = write(connfd, buf, stream.bytes_written);
        if (bytesWritten == -1){
            log_warn("Failed to send response to Omicontrol: %s", strerror(errno));
        }
    }
}

/** called to handle when Omicontrol disconnects */
static void client_disconnected(void){
    log_info("Client has disconnected. Restarting socket server...");
    close(sockfd);
    close(connfd);
    connfd = -1;
    totalFailures = 0;
    init_tcp_socket();
}

/** reads and processes Omicontrol messages. Non-blocking (or at least, as much as is possible). */
static void read_remote_messages(void){
    // read potential protobuf command from Omicontrol socket, recycling buffer
    // https://stackoverflow.com/a/3054519/5007892
    int availableBytes = 0;
    ioctl(connfd, FIONREAD, &availableBytes);

    if (availableBytes > 0) {
        uint8_t *buf = malloc(availableBytes + 64);
        ssize_t readBytes = recv(connfd, buf, availableBytes, MSG_WAITALL);
        if (readBytes == -1){
            log_warn("Failed to read remote message: %s", strerror(readBytes));
            return;
        }
        // printf("recv() return code: %d\n", readBytes);

        pb_istream_t inputStream = pb_istream_from_buffer(buf, availableBytes + 64);
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
                    log_trace("Object %s, min (%d,%d,%d), max (%d,%d,%d)", fieldObjToString[obj], min[0], min[1], min[2], max[0], max[1], max[2]);
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
                log_debug("Received CMD_THRESHOLDS_SELECT, new field object is %s", fieldObjToString[selectedFieldObject]);
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_THRESHOLDS_SET: {
                // do some magic maths to figure out where in the thresholds array we need to access (NOLINTNEXTLINE)
                int index = (selectedFieldObject - 1) * 2 + (!message.minMax);
                thresholds[index][message.colourChannel] = message.value;
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_THRESHOLDS_WRITE_DISK: {
                log_debug("Received CMD_THRESHOLDS_WRITE_DISK");
                utils_write_thresholds_disk();
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_SLEEP_ENTER: {
                log_debug("Received CMD_SLEEP_ENTER, going to enter sleep mode");
                utils_sleep_enter();
                RD_SEND_OK_RESPONSE;
                // wait for Omicontrol to disconnect itself first, so we don't cause the unexpected disconnect message
                sleep(1);
                // since we're not sending messages anymore as we're sleeping (where we would usually check to see if
                // the client disconnected with SIGPIPE), we have to signal the disconnect to the rest of the code manually
                client_disconnected();
                break;
            }
            case CMD_SET_SEND_FRAMES: {
                log_debug("Received CMD_SET_SEND_FRAMES with value: %s", message.isEnabled ? "true" : "false");
                sendDebugFrames = message.isEnabled;
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_RELOAD_CONFIG: {
                log_debug("Received CMD_RELOAD_CONFIG, reloading config from disk");
                utils_reload_config();
                RD_SEND_OK_RESPONSE;
                break;
            }
            case CMD_MOVE_TO_XY ... CMD_MOVE_ORIENT: {
                log_debug("Received robot command (id %d), forwarding to ESP32", message.messageId);
                ESP32DebugCommand fwdCmd = {0};
                fwdCmd.msgId = message.messageId;
                fwdCmd.robotId = message.robotId;

                // copy across data if necessary
                if (message.messageId == CMD_MOVE_TO_XY){
                    fwdCmd.x = message.coords.x;
                    fwdCmd.y = message.coords.y;
                } else if (message.messageId == CMD_MOVE_ORIENT){
                    fwdCmd.orientation = ROUND2INT(message.orientation);
                }

                uint8_t msgBuf[128] = {0};
                pb_ostream_t stream = pb_ostream_from_buffer(msgBuf, 128);
                if (!pb_encode(&stream, ESP32DebugCommand_fields, &fwdCmd)){
                    log_error("Failed to encode debug command message to ESP32: %s", PB_GET_ERROR(&stream));
                    free(buf);
                    return;
                }
                comms_uart_send(MSG_DEBUG_CMD, msgBuf, stream.bytes_written);
                RD_SEND_OK_RESPONSE;
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

/**
 * Encodes the given JPEG images into a Protocol Buffer and dispatches it over the TCP socket, if connected.
 */
static void encode_and_send(uint8_t *camImg, unsigned long camImgSize, uint8_t *threshImg, unsigned long threshImgSize, frame_entry_t *entry){
    DebugFrame msg = DebugFrame_init_zero;
    size_t bufSize = camImgSize + threshImgSize + 32768; // the buffer size is the image sizes + 32KB of extra protobuf data
    uint8_t *buf = malloc(bufSize); // we'll malloc this since we won't ever send the garbage on the end
    pb_ostream_t stream = pb_ostream_from_buffer(buf, bufSize);

    ///////////////////////////////////////////
    /// fill the protocol buffer with data  ///
    //////////////////////////////////////////
    pthread_mutex_lock(&localiserMutex);
    if (sendDebugFrames) {
        // we're in the camera view
        // TODO note that this 9600 value will need to be updated if the protobuf field size is changed
        memcpy(msg.defaultImage.bytes, camImg, MIN(camImgSize, 96000));
        memcpy(msg.ballThreshImage.bytes, threshImg, MIN(threshImgSize, 96000));
        msg.defaultImage.size = camImgSize;
        msg.ballThreshImage.size = threshImgSize;
    } else {
        // we're in the field view so don't send anything to save bandwidth
        msg.defaultImage.size = 0;
        msg.ballThreshImage.size = 0;
    }
    msg.temperature = (float) cpuTemperature;
    // TODO make the msg use RDPointF instead of integers
    msg.ballCentroid.x = ROUND2INT(entry->ballCentroid.x);
    msg.ballCentroid.y = ROUND2INT(entry->ballCentroid.y);
    msg.ballRect = entry->ballRect;
    msg.fps = entry->fps;
    msg.frameWidth = width;
    msg.frameHeight = height;
#if VISION_CROP_ENABLED
    RDRect cropRect = {visionCropRect[0], visionCropRect[1], visionCropRect[2], visionCropRect[3]};
#else
    RDRect cropRect = {0, 0, videoWidth, videoHeight};
#endif
    msg.cropRect = cropRect;
    msg.mirrorRadius = visionMirrorRadius;

    msg.ballPos.x = entry->objectData.ballAbsX;
    msg.ballPos.y = entry->objectData.ballAbsY;

    msg.yellowGoalPos.x = entry->objectData.goalYellowAbsX;
    msg.yellowGoalPos.y = entry->objectData.goalYellowAbsY;

    msg.blueGoalPos.x = entry->objectData.goalBlueAbsX;
    msg.blueGoalPos.y = entry->objectData.goalBlueAbsY;

    msg.isBallKnown = entry->objectData.ballExists;
    msg.isBlueKnown = entry->objectData.goalBlueExists;
    msg.isYellowKnown = entry->objectData.goalYellowExists;

    msg.robots[0].orientation = lastSensorData.orientation;
    // TODO set last FSM state only if not empty
    // TODO set other robot position from bluetooth

    remote_debug_localiser_provide(&msg);
    pthread_mutex_unlock(&localiserMutex);

    RDMsgFrame wrapper = RDMsgFrame_init_zero;
    wrapper.frame = msg;
    wrapper.whichMessage = 1;

    if (!pb_encode_delimited(&stream, RDMsgFrame_fields, &wrapper)){
        log_error("Protobuf encode failed: %s (allocated size was: %d)", PB_GET_ERROR(&stream), bufSize);
        free(buf);
        return;
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
                TJSAMP_420, REMOTE_JPEG_QUALITY, TJFLAG_FASTDCT);
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

    int result = compress2(outBuf, &outSize, inBuffer, inSize, REMOTE_COMPRESS_LEVEL);
    if (result != Z_OK){
        log_error("Failed to zlib compress buffer, error: %d", result);
        return NULL;
    } else {
        *outputSize = outSize;
        return outBuf;
    }
}

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
            pthread_testcancel();
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
        pthread_testcancel();

        //printf("RD encode took: %.2f ms\n", utils_time_millis() - begin);
    }
    return NULL;
}

/** reads the CPU temperature every REMOTE_TEMP_REPORTING_INTERVAL seconds **/
static void *thermal_thread(void *arg){
    log_trace("Thermal thread started");

    while (true){
        // general idea from https://www.raspberrypi.org/forums/viewtopic.php?t=170112#p1091895
        // this also works on the jetson :)
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
        double tempDegrees = (double) tempLong / 1000.0f;
        cpuTemperature = tempDegrees;
        // log_debug("Temperature is %.2f degrees", tempDegrees);

        if (tempDegrees >= 95.0f && !thermalThrottling){
            log_warn("CPU has reached dangerously high temperatures (current temp: %.2f degrees, Celeron max is 105 degrees)", tempDegrees);
            thermalThrottling = true;
        } else if (tempDegrees <= 80.0f && thermalThrottling){
            log_info("CPU has returned to nominal temperature ranges (current temp: %.2f degrees)", tempDegrees);
            thermalThrottling = false;
        }

        sleep(REMOTE_TEMP_REPORTING_INTERVAL);
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
    serverAddr.sin_port = htons(REMOTE_PORT);
    // https://stackoverflow.com/a/24194999/5007892
    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    if (bind(sockfd, &serverAddr, sizeof(serverAddr)) != 0){
        log_error("Failed to bind TCP socket: %s", strerror(errno));
        // make this error really obvous since it's breaks Omicontrol and I freak out every time that happens
        for (int i = 0; i < 5; i++) {
            log_error("\n========================================\nATTENTION: OMICONTROL WILL NOT WORK!!!!\nPlease wait some time"
                    " and then relaunch Omicam.\n========================================");
            sleep(1);
        }
        return NULL;
    }
    if (listen(sockfd, 4) != 0){
        log_error("Failed to listen TCP socket: %s", strerror(errno));
        return NULL;
    }
    clientAddrLen = sizeof(clientAddr);
    log_trace("Awaiting client connection to TCP socket");

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
        utils_sleep_exit();
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
    if (!rpa_queue_create(&frameQueue, 6)){
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
    log_info("Remote debugger initialised successfully");
}

void remote_debug_post(uint8_t *camFrame, uint8_t *threshFrame, RDRect ballRect, RDPointF ballCentroid, int32_t fps,
        int32_t w, int32_t h, ObjectData objectData){
#if !REMOTE_ALWAYS_SEND
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
    entry->fps = fps;
    entry->objectData = objectData;

    // will this work?
    width = w;
    height = h;

    if (!rpa_queue_trypush(frameQueue, entry)){
        if (!wasError){
            wasError = true;
            log_error("Failed to push new frame to queue. This may indicate a hang, performance issue or a busy network.");
        }
        free(camFrame);
        free(entry);
        free(threshFrame);

        if (totalFailures++ >= 75 && remote_debug_is_connected()){
            log_error("Too many failures, going to kick currently connected client!");
            client_disconnected();
        }
    } else {
        if (wasError){
            log_info("Successfully pushed new frame after %d failures", totalFailures);
            wasError = false;
        }
        totalFailures = 0;
    }
}

void remote_debug_dispose(void){
    log_trace("Disposing remote debugger");
    // FIXME if you are connected to remote when this is called, you get a segfault in libturbojpeg memcpy somewhere (see ASan output)
    pthread_cancel(frameThread);
    pthread_cancel(tcpThread);
    pthread_cancel(thermalThread);
    pthread_join(frameThread, NULL);
    pthread_join(tcpThread, NULL);
    pthread_join(thermalThread, NULL);
    rpa_queue_destroy(frameQueue);
    tjDestroy(compressor);
    close(sockfd);
    close(connfd);
    connfd = -1;
}

bool remote_debug_is_connected(void){
#if REMOTE_ALWAYS_SEND
    return true;
#else
    return connfd != -1;
#endif
}