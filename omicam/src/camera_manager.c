#include <raspicam/RaspiCamControl.h>
#include <raspicam/RaspiCommonSettings.h>
#include <iniparser/iniparser.h>
#include <raspicam/RaspiHelpers.h>
#include <log/log.h>
#include <stdbool.h>
#include "camera_manager.h"
#include "bcm_host.h"
#include "interface/vcos/vcos.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "defines.h"
#include "utils.h"
#include "remote_debug.h"
#include "blob_detection.h"
#include <math.h>

// Uses Broadcom's MMAL (somehow faster than omxcam) to decode camera frames and pipe them off to the GPU
// Much of this code is based on RaspiVidYUV:
// https://github.com/raspberrypi/userland/blob/master/host_applications/linux/apps/raspicam/RaspiVidYUV.c
// which is under the following license:

// Copyright (c) 2018, Raspberry Pi (Trading) Ltd.
// Copyright (c) 2014, DSP Group Ltd.
// Copyright (c) 2014, James Hughes
// Copyright (c) 2013, Broadcom Europe Ltd.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the copyright holder nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


static RASPICAM_CAMERA_PARAMETERS cameraParameters;
static RASPICOMMONSETTINGS_PARAMETERS commonSettings;
static MMAL_COMPONENT_T *cameraComponent; // Pointer to the camera component
static MMAL_POOL_T *cameraPool; // Pointer to the pool of buffers used by camera video port
static int framerate = 60;
static MMAL_PORT_T *cameraVideoPort = NULL;
// it seems that the MMAL will post useless buffers that cause the app to hang on exit, hence this value to ignore them
// if we are currently disposing the camera manager
static _Atomic bool ignoreCallback = false;
#if ENABLE_DIAGNOSTICS
static double lastFrameTime = 0.0;
#endif

static bool create_camera_component(void){
    MMAL_COMPONENT_T *camera = 0;
    MMAL_ES_FORMAT_T *format;
    MMAL_PORT_T *video_port = NULL, *still_port = NULL;
    MMAL_STATUS_T status;
    MMAL_POOL_T *pool;

    /* Create the component */
    status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
    if (status != MMAL_SUCCESS){
        log_error("Failed to create camera component: %d", status);
        goto error;
    }

    MMAL_PARAMETER_INT32_T camera_num =
            {{MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, commonSettings.cameraNum};
    status = mmal_port_parameter_set(camera->control, &camera_num.hdr);

    if (status != MMAL_SUCCESS){
        log_error("Could not select camera : error %d", status);
        goto error;
    }

    if (!camera->output_num){
        status = MMAL_ENOSYS;
        log_error("Camera doesn't have output ports");
        goto error;
    }

    status = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, commonSettings.sensor_mode);

    if (status != MMAL_SUCCESS){
        log_error("Could not set sensor mode : error %d", status);
        goto error;
    }

    video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
    still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

    // Enable the camera, and tell it its control callback function
    status = mmal_port_enable(camera->control, default_camera_control_callback);

    if (status != MMAL_SUCCESS){
        log_error("Unable to enable control port : error %d", status);
        goto error;
    }

    //  set up the camera configuration
    {
        MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
                {
                        { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
                        .max_stills_w = commonSettings.width,
                        .max_stills_h = commonSettings.height,
                        .stills_yuv422 = 0,
                        .one_shot_stills = 0,
                        .max_preview_video_w = commonSettings.width,
                        .max_preview_video_h = commonSettings.height,
                        .num_preview_video_frames = 3,
                        .stills_capture_circular_buffer_height = 0,
                        .fast_preview_resume = 0,
                        .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
                };
        mmal_port_parameter_set(camera->control, &cam_config.hdr);
    }

    // skip preview port for now (unless it breaks in which case we'll include it)

    format = video_port->format;

    if(cameraParameters.shutter_speed > 6000000){
        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
                                                { 50, 1000 }, {166, 1000}
        };
        mmal_port_parameter_set(video_port, &fps_range.hdr);
    } else if(cameraParameters.shutter_speed > 1000000) {
        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
                                                { 167, 1000 }, {999, 1000}
        };
        mmal_port_parameter_set(video_port, &fps_range.hdr);
    }

#if USE_RGB
        format->encoding = mmal_util_rgb_order_fixed(still_port) ? MMAL_ENCODING_RGB24 : MMAL_ENCODING_BGR24;
        format->encoding_variant = 0;  //Irrelevant when not in opaque mode
        log_trace("Using RGB encoding");
#else
        format->encoding = MMAL_ENCODING_I420;
        format->encoding_variant = MMAL_ENCODING_I420;
        log_trace("Using YUV encoding");
#endif

    format->es->video.width = VCOS_ALIGN_UP(commonSettings.width, 32);
    format->es->video.height = VCOS_ALIGN_UP(commonSettings.height, 16);
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = commonSettings.width;
    format->es->video.crop.height = commonSettings.height;
    format->es->video.frame_rate.num = framerate;
    format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

    status = mmal_port_format_commit(video_port);

    if (status != MMAL_SUCCESS){
        log_error("camera video format couldn't be set");
        goto error;
    }

    // Ensure there are enough buffers to avoid dropping frames
    if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
        video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

    status = mmal_port_parameter_set_boolean(video_port, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
    if (status != MMAL_SUCCESS){
        log_error("Failed to select zero copy");
        goto error;
    }

    /* Enable component */
    status = mmal_component_enable(camera);

    if (status != MMAL_SUCCESS){
        log_error("Failed to enable camera component. Perhaps it's already in use?");
        goto error;
    }

    raspicamcontrol_set_all_parameters(camera, &cameraParameters);

    /* Create pool of buffer headers for the output port to consume */
    pool = mmal_port_pool_create(video_port, video_port->buffer_num, video_port->buffer_size);

    if (!pool){
        log_error("Failed to create buffer header pool for camera still port %s", still_port->name);
    }

    cameraPool = pool;
    cameraComponent = camera;

    log_trace("Initialised camera component successfully");
    return status;

    error:
        log_warn("An error occurred in create_camera_component, destroying camera");
        if (camera != NULL){
            mmal_component_destroy(camera);
            return false;
        }
        return false;
}

static uint32_t frames = 0;
static void camera_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer){
    if (ignoreCallback) goto end;

    // FIXME later on we may need to copy processedFrame if there's ever any threading issues
    uint8_t *processedFrame = blob_detector_post(buffer, commonSettings.width, commonSettings.height);

#if DEBUG_ENABLED
    if (frames++ % DEBUG_FRAME_EVERY == 0 && remote_debug_is_connected()){
        // for the remote debugger, frames are processed on another thread so we must copy the buffer before posting it
        // the buffer will be automatically freed by the encoding thread once processed successfully
        uint8_t *camFrame = malloc(buffer->length);
        memcpy(camFrame, buffer->data, buffer->length);
        remote_debug_post_frame(camFrame, processedFrame);
    } else {
        // not needed by remote debugger, so just free it
        free(processedFrame);
    }
#else
    free(processedFrame);
#endif

    end:
    // handle MMAL stuff as well
    mmal_buffer_header_release(buffer);
    if (port->is_enabled){
        MMAL_STATUS_T status = MMAL_SUCCESS;
        MMAL_BUFFER_HEADER_T *new_buffer = mmal_queue_get(cameraPool->queue);

        if (new_buffer)
            status = mmal_port_send_buffer(port, new_buffer);

        if (!new_buffer || status != MMAL_SUCCESS) {
            log_error("Unable to return a buffer to the camera port");
        }
    }

#if ENABLE_DIAGNOSTICS
    double currentTime = utils_get_millis();
    double diff = fabs(currentTime - lastFrameTime);
    printf("Last frame was: %.2f ms ago (%.2f fps)\n", diff, 1000.0 / diff);
    lastFrameTime = currentTime;
#endif
}

/** called by camera_manager_init() to set the cam settings based on what's in the INI file */
static void cam_set_settings(dictionary *config){
    // set default settings
    raspicommonsettings_set_defaults(&commonSettings);

    commonSettings.width = iniparser_getint(config, "VideoSettings:width", 1280);
    commonSettings.height = iniparser_getint(config, "VideoSettings:height", 720);
    commonSettings.verbose = true;
    framerate = iniparser_getint(config, "VideoSettings:framerate", 60);

    raspicamcontrol_set_defaults(&cameraParameters);

    blob_detector_init(commonSettings.width, commonSettings.height);
    remote_debug_init(commonSettings.width, commonSettings.height);
}

void camera_manager_init(dictionary *config){
    bcm_host_init();
    cam_set_settings(config);

    get_sensor_defaults(commonSettings.cameraNum, commonSettings.camera_name, &commonSettings.width, &commonSettings.height);
    log_debug("Camera num: %d, camera name: %s, width: %d, height: %d", commonSettings.cameraNum, commonSettings.camera_name,
            commonSettings.width, commonSettings.height);
    log_debug("Framerate is: %d fps", framerate);


    log_trace("Creating and connecting components...");
    create_camera_component();
    cameraVideoPort = cameraComponent->output[MMAL_CAMERA_VIDEO_PORT];
    MMAL_STATUS_T status;

    // here RaspiVidYUV opens the output file, but in our case we don't want output so instead I suppose we should
    // use fmemopen but in actuality we want to pass it off to the GPU as a texture so idk
    status = mmal_port_enable(cameraVideoPort, camera_buffer_callback);

    if (status != MMAL_SUCCESS){
        log_error("Failed to setup camera output");
        goto error;
    }

    // Send all the buffers to the camera video port
    {
        uint32_t num = mmal_queue_length(cameraPool->queue);
        uint32_t q;
        for (q = 0; q < num; q++){
            MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(cameraPool->queue);

            if (!buffer) {
                log_error("Unable to get a required buffer %d from pool queue", q);
            }

            if (mmal_port_send_buffer(cameraVideoPort, buffer) != MMAL_SUCCESS) {
                log_error("Unable to send a buffer to camera video port (%d)", q);
            }
        }
    }

    log_debug("MMAL and camera initialised successfully");
    return;

    error:
        log_warn("An error occurred initialising MMAL, shutting down");
        camera_manager_dispose();
}

void camera_manager_capture(void){
    log_info("Starting capture");
    while (true){
        if (mmal_port_parameter_set_boolean(cameraVideoPort, MMAL_PARAMETER_CAPTURE, MMAL_TRUE) != MMAL_SUCCESS){
            log_error("u wot");
        }

        // We never return from this. Expect a ctrl-c to exit.
        while (1)
            // Have a sleep so we don't hog the CPU.
            vcos_sleep(10000);
    }
}

void camera_manager_dispose(void){
    log_trace("Disposing camera manager");
    ignoreCallback = true;
    if (mmal_port_parameter_set_boolean(cameraVideoPort, MMAL_PARAMETER_CAPTURE, MMAL_FALSE) != MMAL_SUCCESS){
        log_error("Failed to stop capture, segfault incoming D:");
    }
    check_disable_port(cameraVideoPort);
    if (cameraComponent != NULL){
        mmal_component_disable(cameraComponent);
        mmal_component_destroy(cameraComponent);
        cameraComponent = NULL;
    }
}